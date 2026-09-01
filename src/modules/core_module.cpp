#include "core_module.h"

#include "../host/host_thread_check.h"
#include "../lua/lua_runtime.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
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

// native_core.get_root_path() -> string
// 获取项目根目录的绝对路径。
static int l_get_root_path(lua_State *p_L) {
	godot::String path = godot::ProjectSettings::get_singleton()->globalize_path("res://");
	lua_pushstring(p_L, path.utf8().get_data());
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

// native_core.get_unique_id() -> string
// 获取设备唯一标识符。
// 注意：该字符串在重装系统、升级或修改硬件后可能变化，不可用于持久数据加密；也可能被外部程序伪造，不可用于安全校验。
static int l_get_unique_id(lua_State *p_L) {
	godot::String unique_id = godot::OS::get_singleton()->get_unique_id();
	lua_pushstring(p_L, unique_id.utf8().get_data());
	return 1;
}

// native_core.get_locale() -> string
// 获取宿主操作系统的区域设置（locale），与 Godot 的 OS.get_locale() 一致。
// 返回：形如 language_Script_COUNTRY_VARIANT@extra 的字符串，language 之后的部分均为可选。
// 注意：如需仅获取 2 或 3 字母语言代码，请使用 OS.get_locale_language()。
static int l_get_locale(lua_State *p_L) {
	godot::String locale = godot::OS::get_singleton()->get_locale();
	lua_pushstring(p_L, locale.utf8().get_data());
	return 1;
}

static const luaL_Reg core_funcs[] = {
	{"bind_update", l_bind_update},
	{"bind_shutdown", l_bind_shutdown},
	{"bind_fatal", l_bind_fatal},
	{"quit", l_quit},
	{"set_time_scale", l_set_time_scale},
	{"get_time_scale", l_get_time_scale},
	{"get_root_path", l_get_root_path},
	{"string_hash", l_string_hash},
	{"get_unique_id", l_get_unique_id},
	{"get_locale", l_get_locale},
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

} // namespace luagd
