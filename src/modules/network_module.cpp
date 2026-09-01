#include "network_module.h"

#include "../host/host_thread_check.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 请求记录
struct RequestRecord {
	int32_t id;
	godot::HTTPRequest *request;
	class NetworkSignalReceiver *receiver;
	bool completed;
	int64_t result;
	int64_t response_code;
	godot::PackedByteArray body;
};

// 模块级静态数据
static godot::HashMap<int32_t, RequestRecord> requests;
static int32_t next_id = 1;
static godot::Node *network_root = nullptr;
static bool initialized = false;

// 获取请求记录，不存在时打印错误
static RequestRecord *get_request(int32_t p_id, const char *p_func_name) {
	if (!requests.has(p_id)) {
		godot::UtilityFunctions::printerr("native_network.", p_func_name, ": invalid id ", p_id);
		return nullptr;
	}
	return &requests[p_id];
}

// 立即销毁节点，避免退出同帧时 queue_free() 来不及释放资源。
static void free_node_immediately(godot::Node *p_node) {
	if (p_node == nullptr) {
		return;
	}

	if (p_node->get_parent() != nullptr) {
		p_node->get_parent()->remove_child(p_node);
	}
	memdelete(p_node);
}

// ============================================================================
// 信号接收器
// ============================================================================

// NetworkSignalReceiver：接收 HTTPRequest 的 request_completed 信号，
// 保存传输结果、HTTP 状态码与响应体。
class NetworkSignalReceiver : public godot::Object {
	GDCLASS(NetworkSignalReceiver, godot::Object);

public:
	int32_t request_id = 0;

	// 契约：request_completed(result, response_code, headers, body)
	void on_request_completed(int64_t p_result, int64_t p_response_code,
			const godot::PackedStringArray &p_headers, const godot::PackedByteArray &p_body);

protected:
	static void _bind_methods();
};

void NetworkSignalReceiver::_bind_methods() {
	godot::ClassDB::bind_method(
			godot::D_METHOD("on_request_completed", "result", "response_code", "headers", "body"),
			&NetworkSignalReceiver::on_request_completed);
}

void NetworkSignalReceiver::on_request_completed(int64_t p_result, int64_t p_response_code,
		const godot::PackedStringArray &p_headers, const godot::PackedByteArray &p_body) {
	if (!requests.has(request_id)) {
		return;
	}
	RequestRecord &record = requests[request_id];
	record.completed = true;
	record.result = p_result;
	record.response_code = p_response_code;
	record.body = p_body;
}

// ============================================================================
// 模块初始化
// ============================================================================

// init(root_name) -> bool
// 初始化网络模块，创建容器节点。
// 必须在场景树就绪后调用（如第一帧 update 中）。
static int l_init(lua_State *p_L) {
	if (initialized) {
		lua_pushboolean(p_L, true);
		return 1;
	}

	const char *root_name = luaL_optstring(p_L, 1, "_NetworkRoot");

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(
		godot::Engine::get_singleton()->get_main_loop()
	);
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_network.init: SceneTree not available");
		lua_pushboolean(p_L, false);
		return 1;
	}

	network_root = memnew(godot::Node);
	network_root->set_name(root_name);
	tree->get_root()->add_child(network_root);
	initialized = true;

	lua_pushboolean(p_L, true);
	return 1;
}

// is_initialized() -> bool
// 查询模块是否已初始化。
static int l_is_initialized(lua_State *p_L) {
	lua_pushboolean(p_L, initialized);
	return 1;
}

// ============================================================================
// 请求生命周期
// ============================================================================

// 清理请求节点与其信号接收器。
static void free_request_record(const RequestRecord *p_rec) {
	if (p_rec == nullptr) {
		return;
	}

	if (p_rec->receiver != nullptr) {
		p_rec->request->disconnect("request_completed", godot::Callable(p_rec->receiver, "on_request_completed"));
		memdelete(p_rec->receiver);
	}
	free_node_immediately(p_rec->request);
}

// http_post(url, content_type, authorization, body) -> id, error
// 发起 HTTP POST 请求，内部创建 HTTPRequest 节点挂载到容器。
// authorization 为空字符串时不携带 Authorization 头。
// 返回请求 ID 与错误码；发起失败时返回 id=-1。
static int l_http_post(lua_State *p_L) {
	if (!initialized) {
		godot::UtilityFunctions::printerr("native_network.http_post: not initialized, call init() first");
		lua_pushinteger(p_L, -1);
		lua_pushinteger(p_L, (int64_t)godot::Error::ERR_UNCONFIGURED);
		return 2;
	}

	const char *url = luaL_checkstring(p_L, 1);
	const char *content_type = luaL_checkstring(p_L, 2);
	const char *authorization = luaL_optstring(p_L, 3, "");
	const char *body = luaL_checkstring(p_L, 4);

	RequestRecord rec;
	rec.id = next_id++;
	rec.completed = false;
	rec.result = 0;
	rec.response_code = 0;
	rec.body = godot::PackedByteArray();

	// 可选超时（秒），0.0 表示不超时；默认 10.0 防止请求永久卡住。
	double timeout = luaL_optnumber(p_L, 5, 10.0);

	rec.request = memnew(godot::HTTPRequest);
	rec.request->set_timeout((float)timeout);
	network_root->add_child(rec.request);

	rec.receiver = memnew(NetworkSignalReceiver);
	rec.receiver->request_id = rec.id;
	rec.request->connect("request_completed", godot::Callable(rec.receiver, "on_request_completed"));

	godot::PackedStringArray headers;
	headers.push_back(godot::String("Content-Type: ") + content_type);
	if (authorization[0] != '\0') {
		headers.push_back(godot::String("Authorization: ") + authorization);
	}

	godot::Error err = rec.request->request(
			godot::String(url), headers, godot::HTTPClient::METHOD_POST, godot::String(body));
	if (err != godot::Error::OK) {
		godot::UtilityFunctions::printerr("native_network.http_post: request failed, error=", (int64_t)err);
		free_request_record(&rec);
		lua_pushinteger(p_L, -1);
		lua_pushinteger(p_L, (int64_t)err);
		return 2;
	}

	requests[rec.id] = rec;
	lua_pushinteger(p_L, rec.id);
	lua_pushinteger(p_L, (int64_t)godot::Error::OK);
	return 2;
}

// get_http_post_status(request_id) -> completed, result, response_code, body
// 查询指定请求的完成状态、传输结果、HTTP 状态码与响应体。
// result 为 Godot HTTPRequest.Result，0 表示传输成功。
// response_code 为 HTTP 状态码，传输层失败时通常为 0。
// body 为响应体字节串；未完成或无效 id 时返回空串。
static int l_get_http_post_status(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	RequestRecord *rec = get_request(id, "get_http_post_status");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushlstring(p_L, "", 0);
		return 4;
	}

	lua_pushboolean(p_L, rec->completed);
	lua_pushinteger(p_L, rec->result);
	lua_pushinteger(p_L, rec->response_code);
	lua_pushlstring(p_L, (const char *)rec->body.ptr(), rec->body.size());
	return 4;
}

// destroy_request(request_id) -> void
// 销毁请求，断开信号并从场景树移除节点。
// Lua 侧不依赖 GC，请求必须显式销毁。
static int l_destroy_request(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	RequestRecord *rec = get_request(id, "destroy_request");
	if (rec == nullptr) {
		return 0;
	}

	free_request_record(rec);
	requests.erase(id);
	return 0;
}

// http_post_sync(url, content_type, authorization, body, timeout?) -> bool
// 阻塞式 POST，仅确保请求字节发出，不读响应体。仅供关闭时同步 flush 使用。
// 约束：仅支持 http://，遇 https:// 直接失败。timeout 单位秒，默认 1.0。
// 注意：不使用 HTTPRequest，因其依赖场景树逐帧 process，关闭时无法驱动。
static int l_http_post_sync(lua_State *p_L) {
	const char *url = luaL_checkstring(p_L, 1);
	const char *content_type = luaL_checkstring(p_L, 2);
	const char *authorization = luaL_optstring(p_L, 3, "");
	const char *body = luaL_checkstring(p_L, 4);
	double timeout = luaL_optnumber(p_L, 5, 1.0);

	godot::String full_url(url);
	if (!full_url.begins_with("http://")) {
		godot::UtilityFunctions::printerr("native_network.http_post_sync: only http:// supported, url=", full_url);
		lua_pushboolean(p_L, false);
		return 1;
	}

	// 解析 host / port / path，query 拼入 path
	godot::String rest = full_url.substr(7);
	int64_t slash = rest.find("/");
	godot::String host_port = (slash < 0) ? rest : rest.substr(0, slash);
	godot::String path = (slash < 0) ? godot::String("/") : rest.substr(slash);
	int64_t colon = host_port.find(":");
	godot::String host = (colon < 0) ? host_port : host_port.substr(0, colon);
	int32_t port = (colon < 0) ? 80 : (int32_t)host_port.substr(colon + 1).to_int();

	godot::Ref<godot::HTTPClient> client = memnew(godot::HTTPClient);
	uint64_t start_ms = godot::Time::get_singleton()->get_ticks_msec();
	uint64_t timeout_ms = (uint64_t)(timeout * 1000.0);

	if (client->connect_to_host(host, port) != godot::Error::OK) {
		godot::UtilityFunctions::printerr("native_network.http_post_sync: connect failed, host=", host);
		lua_pushboolean(p_L, false);
		return 1;
	}

	// 轮询至已连接
	while (true) {
		client->poll();
		godot::HTTPClient::Status status = client->get_status();
		if (status == godot::HTTPClient::STATUS_CONNECTED) {
			break;
		}
		if (status != godot::HTTPClient::STATUS_CONNECTING
				&& status != godot::HTTPClient::STATUS_RESOLVING) {
			godot::UtilityFunctions::printerr("native_network.http_post_sync: connect status error, status=", (int64_t)status);
			lua_pushboolean(p_L, false);
			return 1;
		}
		if (godot::Time::get_singleton()->get_ticks_msec() - start_ms > timeout_ms) {
			godot::UtilityFunctions::printerr("native_network.http_post_sync: connect timeout");
			lua_pushboolean(p_L, false);
			return 1;
		}
		godot::OS::get_singleton()->delay_msec(1);
	}

	// 发送请求
	godot::PackedStringArray headers;
	headers.push_back(godot::String("Content-Type: ") + content_type);
	if (authorization[0] != '\0') {
		headers.push_back(godot::String("Authorization: ") + authorization);
	}
	if (client->request(godot::HTTPClient::METHOD_POST, path, headers, godot::String(body)) != godot::Error::OK) {
		godot::UtilityFunctions::printerr("native_network.http_post_sync: request failed");
		lua_pushboolean(p_L, false);
		return 1;
	}

	// 轮询至请求发出（状态离开 REQUESTING）
	while (client->get_status() == godot::HTTPClient::STATUS_REQUESTING) {
		client->poll();
		if (godot::Time::get_singleton()->get_ticks_msec() - start_ms > timeout_ms) {
			godot::UtilityFunctions::printerr("native_network.http_post_sync: request timeout");
			lua_pushboolean(p_L, false);
			return 1;
		}
		godot::OS::get_singleton()->delay_msec(1);
	}

	godot::HTTPClient::Status final_status = client->get_status();
	client->close();
	// BODY / CONNECTED 视为请求字节已发出
	bool ok = (final_status == godot::HTTPClient::STATUS_BODY
			|| final_status == godot::HTTPClient::STATUS_CONNECTED);
	if (!ok) {
		godot::UtilityFunctions::printerr("native_network.http_post_sync: unexpected final status=", (int64_t)final_status);
	}
	lua_pushboolean(p_L, ok);
	return 1;
}

// ============================================================================
// 模块注册
// ============================================================================

static const luaL_Reg network_funcs[] = {
	// 模块初始化
	{"init", l_init},
	{"is_initialized", l_is_initialized},

	// 请求生命周期
	{"http_post", l_http_post},
	{"get_http_post_status", l_get_http_post_status},
	{"destroy_request", l_destroy_request},
	{"http_post_sync", l_http_post_sync},

	{nullptr, nullptr}
};

int luaopen_native_network(lua_State *p_L) {
	luaL_newlib(p_L, network_funcs);
	return 1;
}

void network_cleanup() {
	if (!ensure_main_thread("native_network.network_cleanup")) {
		return;
	}

	// GDExtension 反初始化阶段只清理模块记录，场景对象交给引擎统一销毁。
	requests.clear();
	network_root = nullptr;

	next_id = 1;
	initialized = false;
}

void network_register_signal_receivers() {
	GDREGISTER_CLASS(NetworkSignalReceiver);
}

} // namespace luagd