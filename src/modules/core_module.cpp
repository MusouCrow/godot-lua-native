#include "core_module.h"

#include "../host/host_thread_check.h"
#include "../lua/lua_runtime.h"
#include "../lua/packed_lua_archive.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// Lua registry 键名，用于存储回调函数引用
static const char *UPDATE_CALLBACK_KEY = "native_core.update_callback";
static const char *SHUTDOWN_CALLBACK_KEY = "native_core.shutdown_callback";
static const char *FATAL_CALLBACK_KEY = "native_core.fatal_callback";

// 已解析的 lua.dat 归档缓存。使用指针以避免静态初始化 Godot 对象。
// 同一 dat_path 复用，不同 dat_path 替换，LuaRuntime shutdown 时释放。
static PackedLuaArchive *packed_lua_archive = nullptr;
static godot::String *packed_lua_archive_path = nullptr;

static bool ensure_packed_lua_archive(
		const godot::String &p_dat_path,
		godot::String *r_error) {
	if (packed_lua_archive != nullptr &&
			packed_lua_archive_path != nullptr &&
			*packed_lua_archive_path == p_dat_path &&
			packed_lua_archive->is_open()) {
		return true;
	}

	if (packed_lua_archive == nullptr) {
		packed_lua_archive = memnew(PackedLuaArchive);
	}

	if (packed_lua_archive_path == nullptr) {
		packed_lua_archive_path = memnew(godot::String);
	}

	packed_lua_archive->close();
	*packed_lua_archive_path = godot::String();

	if (!packed_lua_archive->open(
			p_dat_path,
			LUA_VERSION_RELEASE_NUM,
			r_error)) {
		return false;
	}

	*packed_lua_archive_path = p_dat_path;
	return true;
}

// native_core.bind_update(func) -> void
// 绑定 update 回调函数。
// func: 接收 delta 参数的函数。
static int l_bind_update(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_core.bind_update: expected 1 argument (function), got ", argc);
		return 0;
	}

	if (!lua_isfunction(p_L, 1)) {
		godot::UtilityFunctions::printerr("native_core.bind_update: argument must be a function");
		return 0;
	}

	// 将函数存储到 registry
	lua_pushvalue(p_L, 1);  // 复制函数到栈顶
	lua_setfield(p_L, LUA_REGISTRYINDEX, UPDATE_CALLBACK_KEY);

	return 0;
}

// native_core.bind_shutdown(func) -> void
// 绑定 shutdown 回调函数。
// func: 无参函数。
static int l_bind_shutdown(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_core.bind_shutdown: expected 1 argument (function), got ", argc);
		return 0;
	}

	if (!lua_isfunction(p_L, 1)) {
		godot::UtilityFunctions::printerr("native_core.bind_shutdown: argument must be a function");
		return 0;
	}

	// 将函数存储到 registry
	lua_pushvalue(p_L, 1);  // 复制函数到栈顶
	lua_setfield(p_L, LUA_REGISTRYINDEX, SHUTDOWN_CALLBACK_KEY);

	return 0;
}

// native_core.bind_fatal(func) -> void
// 绑定致命错误善后回调函数。
// func: 接收 message 字符串参数的函数，在 Lua 运行时销毁前被调用。
static int l_bind_fatal(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_core.bind_fatal: expected 1 argument (function), got ", argc);
		return 0;
	}

	if (!lua_isfunction(p_L, 1)) {
		godot::UtilityFunctions::printerr("native_core.bind_fatal: argument must be a function");
		return 0;
	}

	// 将函数存储到 registry
	lua_pushvalue(p_L, 1);  // 复制函数到栈顶
	lua_setfield(p_L, LUA_REGISTRYINDEX, FATAL_CALLBACK_KEY);

	return 0;
}

// native_core.quit(exit_code) -> void
// 请求优雅退出。
// exit_code: 退出码，默认 0。
static int l_quit(lua_State *p_L) {
	int exit_code = (int)luaL_optinteger(p_L, 1, 0);

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(
		godot::Engine::get_singleton()->get_main_loop()
	);
	if (tree) {
		tree->quit(exit_code);
	}
	return 0;
}

// native_core.set_time_scale(scale) -> void
// 设置游戏时间缩放比例。
// scale: 时间缩放倍率，1.0 为正常速度，2.0 为两倍速，0.5 为半速。
static int l_set_time_scale(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_core.set_time_scale: expected 1 argument (number), got ", argc);
		return 0;
	}

	double scale = luaL_checknumber(p_L, 1);
	godot::Engine::get_singleton()->set_time_scale(scale);

	return 0;
}

// native_core.get_time_scale() -> number
// 获取当前游戏时间缩放比例。
static int l_get_time_scale(lua_State *p_L) {
	double scale = godot::Engine::get_singleton()->get_time_scale();
	lua_pushnumber(p_L, scale);
	return 1;
}

// native_core.string_hash(str) -> integer
// 计算字符串的哈希值，与 Godot 的 String.hash() 一致。
// str: 待计算哈希的字符串。
// 返回：32 位哈希数值。
static int l_string_hash(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_core.string_hash: expected 1 argument (string), got ", argc);
		return 0;
	}

	const char *str = luaL_checkstring(p_L, 1);
	godot::String gd_str = str;
	lua_pushinteger(p_L, gd_str.hash());

	return 1;
}

// native_core.load_packed_lua(dat_path, module_name) -> loader, loader_data
// 从 lua.dat 归档中加载一个 Lua 字节码模块。
// 成功：返回 loader 函数与 loader data 字符串，满足 Lua 5.5 searcher 协议。
// 失败：返回 nil 与错误文本。
// 不使用 luaL_error() 抛出归档错误，避免 longjmp 跳过 C++ 局部对象析构。
static int l_load_packed_lua(lua_State *p_L) {
	const char *dat_path_cstr =
			luaL_checkstring(p_L, 1);
	const char *module_name_cstr =
			luaL_checkstring(p_L, 2);

	const godot::String dat_path =
			godot::String::utf8(dat_path_cstr);
	const godot::String module_name =
			godot::String::utf8(module_name_cstr);

	godot::String error;

	if (!ensure_packed_lua_archive(dat_path, &error)) {
		lua_pushnil(p_L);
		lua_pushstring(p_L, error.utf8().get_data());
		return 2;
	}

	const uint8_t *bytecode = nullptr;
	uint64_t bytecode_size = 0;

	if (!packed_lua_archive->get_module(
			module_name,
			&bytecode,
			&bytecode_size)) {
		lua_pushnil(p_L);
		lua_pushfstring(
				p_L,
				"packed Lua module not found: %s",
				module_name_cstr);
		return 2;
	}

	const godot::String chunk_name =
			"@" + module_name.replace(".", "/") + ".lua";
	const godot::CharString chunk_name_utf8 =
			chunk_name.utf8();

	const int load_result = luaL_loadbufferx(
			p_L,
			reinterpret_cast<const char *>(bytecode),
			static_cast<size_t>(bytecode_size),
			chunk_name_utf8.get_data(),
			"b");

	if (load_result != LUA_OK) {
		lua_pushnil(p_L);
		lua_insert(p_L, -2);
		return 2;
	}

	lua_pushstring(p_L, chunk_name_utf8.get_data());
	return 2;
}

static const luaL_Reg core_funcs[] = {
	{"bind_update", l_bind_update},
	{"bind_shutdown", l_bind_shutdown},
	{"bind_fatal", l_bind_fatal},
	{"quit", l_quit},
	{"set_time_scale", l_set_time_scale},
	{"get_time_scale", l_get_time_scale},
	{"string_hash", l_string_hash},
	{"load_packed_lua", l_load_packed_lua},
	{nullptr, nullptr}
};

int luaopen_native_core(lua_State *p_L) {
	luaL_newlib(p_L, core_funcs);
	return 1;
}

int core_call_update(lua_State *p_L, double p_delta) {
	if (!ensure_main_thread("native_core.core_call_update")) {
		return -1;
	}

	// 从 registry 获取回调函数
	lua_getfield(p_L, LUA_REGISTRYINDEX, UPDATE_CALLBACK_KEY);

	if (lua_isnil(p_L, -1)) {
		// 未绑定回调，静默跳过
		lua_pop(p_L, 1);
		return 0;
	}

	if (!lua_isfunction(p_L, -1)) {
		godot::UtilityFunctions::printerr("native_core: update callback is not a function");
		lua_pop(p_L, 1);
		return -1;
	}

	// 压入 delta 参数
	lua_pushnumber(p_L, p_delta);

	// 调用函数（1 个参数，0 个返回值）
	int call_result = LuaRuntime::pcall(
			p_L,
			1,
			0,
			"native_core: update callback error");
	if (call_result != LUA_OK) {
		lua_pop(p_L, 1);
		return call_result;
	}

	return 0;
}

void core_call_shutdown(lua_State *p_L) {
	if (!ensure_main_thread("native_core.core_call_shutdown")) {
		return;
	}

	// 从 registry 获取回调函数
	lua_getfield(p_L, LUA_REGISTRYINDEX, SHUTDOWN_CALLBACK_KEY);

	if (lua_isnil(p_L, -1)) {
		// 未绑定回调，静默跳过
		lua_pop(p_L, 1);
		return;
	}

	if (!lua_isfunction(p_L, -1)) {
		godot::UtilityFunctions::printerr("native_core: shutdown callback is not a function");
		lua_pop(p_L, 1);
		return;
	}

	// 调用函数（0 个参数，0 个返回值）
	int call_result = LuaRuntime::pcall(
			p_L,
			0,
			0,
			"native_core: shutdown callback error");
	if (call_result != LUA_OK) {
		lua_pop(p_L, 1);
		// shutdown 错误只记录为致命，不影响本流程返回
	}
}

// 调用 Lua 的致命错误善后回调。
// p_message: 致命错误文本，传入 Lua 侧用于报错上传。
// 约束：只允许在主线程调用。
// 错误只打印，不影响后续 terminate 与蓝屏显示流程。
void core_call_fatal(lua_State *p_L, const godot::String &p_message) {
	if (!ensure_main_thread("native_core.core_call_fatal")) {
		return;
	}

	// 从 registry 获取回调函数
	lua_getfield(p_L, LUA_REGISTRYINDEX, FATAL_CALLBACK_KEY);

	if (lua_isnil(p_L, -1)) {
		// 未绑定回调，静默跳过
		lua_pop(p_L, 1);
		return;
	}

	if (!lua_isfunction(p_L, -1)) {
		godot::UtilityFunctions::printerr("native_core: fatal callback is not a function");
		lua_pop(p_L, 1);
		return;
	}

	// 压入 message 参数
	lua_pushstring(p_L, p_message.utf8().get_data());

	// 调用函数（1 个参数，0 个返回值）
	int call_result = LuaRuntime::pcall(
			p_L,
			1,
			0,
			"native_core: fatal callback error");
	if (call_result != LUA_OK) {
		lua_pop(p_L, 1);
		// fatal 善后错误只打印，不影响 terminate 与蓝屏流程
	}
}

// 清理 native_core 持有的 Lua archive 缓存。
// 不访问 Lua 栈，可在 lua_close() 前后调用。
void core_cleanup() {
	if (packed_lua_archive != nullptr) {
		godot::memdelete(packed_lua_archive);
		packed_lua_archive = nullptr;
	}

	if (packed_lua_archive_path != nullptr) {
		godot::memdelete(packed_lua_archive_path);
		packed_lua_archive_path = nullptr;
	}
}

} // namespace luagd
