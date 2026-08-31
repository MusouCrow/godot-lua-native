#include "network_module.h"

#include "../host/host_thread_check.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/http_client.hpp>
#include <godot_cpp/classes/http_request.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
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
// 仅按需求标记对应请求完成，不保存结果。
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
	requests[request_id].completed = true;
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

// http_post(url, content_type, body) -> id, error
// 发起 HTTP POST 请求，内部创建 HTTPRequest 节点挂载到容器。
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
	const char *body = luaL_checkstring(p_L, 3);

	RequestRecord rec;
	rec.id = next_id++;
	rec.completed = false;

	rec.request = memnew(godot::HTTPRequest);
	network_root->add_child(rec.request);

	rec.receiver = memnew(NetworkSignalReceiver);
	rec.receiver->request_id = rec.id;
	rec.request->connect("request_completed", godot::Callable(rec.receiver, "on_request_completed"));

	godot::PackedStringArray headers;
	headers.push_back(godot::String("Content-Type: ") + content_type);

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

// is_http_post_completed(request_id) -> bool
// 查询指定请求是否已完成（无论成功或失败）。
static int l_is_http_post_completed(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	RequestRecord *rec = get_request(id, "is_http_post_completed");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, rec->completed);
	return 1;
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

// ============================================================================
// 模块注册
// ============================================================================

static const luaL_Reg network_funcs[] = {
	// 模块初始化
	{"init", l_init},
	{"is_initialized", l_is_initialized},

	// 请求生命周期
	{"http_post", l_http_post},
	{"is_http_post_completed", l_is_http_post_completed},
	{"destroy_request", l_destroy_request},

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