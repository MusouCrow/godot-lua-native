#include "core_module.h"

#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// Lua registry 键名，用于存储回调函数引用
static const char *UPDATE_CALLBACK_KEY = "native_core.update_callback";
static const char *SHUTDOWN_CALLBACK_KEY = "native_core.shutdown_callback";

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

static const luaL_Reg core_funcs[] = {
	{"bind_update", l_bind_update},
	{"bind_shutdown", l_bind_shutdown},
	{nullptr, nullptr}
};

int luaopen_native_core(lua_State *p_L) {
	luaL_newlib(p_L, core_funcs);
	return 1;
}

int core_call_update(lua_State *p_L, double p_delta) {
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
	int call_result = lua_pcall(p_L, 1, 0, 0);
	if (call_result != LUA_OK) {
		const char *err = lua_tostring(p_L, -1);
		godot::String err_msg = "native_core: update callback error: ";
		err_msg += err ? err : "(unknown)";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pop(p_L, 1);
		return call_result;
	}

	return 0;
}

void core_call_shutdown(lua_State *p_L) {
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
	int call_result = lua_pcall(p_L, 0, 0, 0);
	if (call_result != LUA_OK) {
		const char *err = lua_tostring(p_L, -1);
		godot::String err_msg = "native_core: shutdown callback error: ";
		err_msg += err ? err : "(unknown)";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pop(p_L, 1);
		// shutdown 错误只打印，不影响退出流程
	}
}

} // namespace luagd
