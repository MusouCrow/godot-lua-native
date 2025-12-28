#include "display_module.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 主窗口 ID（固定值）
static const int32_t MAIN_WINDOW_ID = 0;

// native_display.window_get_size() -> (w:int, h:int)
// 返回：宽度和高度两个整数；DisplayServer 不可用时返回 (0, 0)。
static int l_window_get_size(lua_State *p_L) {
	godot::DisplayServer *ds = godot::DisplayServer::get_singleton();
	if (ds == nullptr) {
		godot::UtilityFunctions::printerr("native_display.window_get_size: DisplayServer not available");
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		return 2;
	}

	godot::Vector2i size = ds->window_get_size(MAIN_WINDOW_ID);
	lua_pushinteger(p_L, size.x);
	lua_pushinteger(p_L, size.y);
	return 2;
}

// native_display.window_set_size(w:int, h:int) -> rc:int
// 返回：成功返回 0，失败返回 -1。
// 约束：w 和 h 必须为正整数；全屏/最大化模式下无法设置尺寸。
static int l_window_set_size(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_display.window_set_size: expected 2 arguments (w, h), got ", argc);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	if (!lua_isinteger(p_L, 1) || !lua_isinteger(p_L, 2)) {
		godot::UtilityFunctions::printerr("native_display.window_set_size: arguments must be integers");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	int64_t w = lua_tointeger(p_L, 1);
	int64_t h = lua_tointeger(p_L, 2);

	if (w <= 0 || h <= 0) {
		godot::String err_msg = "native_display.window_set_size: invalid size (";
		err_msg += godot::String::num_int64(w);
		err_msg += ", ";
		err_msg += godot::String::num_int64(h);
		err_msg += "), width and height must be > 0";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::DisplayServer *ds = godot::DisplayServer::get_singleton();
	if (ds == nullptr) {
		godot::UtilityFunctions::printerr("native_display.window_set_size: DisplayServer not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 约束：全屏/最大化模式下无法设置窗口尺寸
	godot::DisplayServer::WindowMode mode = ds->window_get_mode(MAIN_WINDOW_ID);
	if (mode == godot::DisplayServer::WINDOW_MODE_FULLSCREEN ||
		mode == godot::DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN ||
		mode == godot::DisplayServer::WINDOW_MODE_MAXIMIZED) {
		const char *mode_name = "unknown";
		switch (mode) {
			case godot::DisplayServer::WINDOW_MODE_FULLSCREEN:
				mode_name = "fullscreen";
				break;
			case godot::DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN:
				mode_name = "exclusive_fullscreen";
				break;
			case godot::DisplayServer::WINDOW_MODE_MAXIMIZED:
				mode_name = "maximized";
				break;
			default:
				break;
		}
		godot::String err_msg = "native_display.window_set_size: cannot set size in ";
		err_msg += mode_name;
		err_msg += " mode";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Vector2i size((int32_t)w, (int32_t)h);
	ds->window_set_size(size, MAIN_WINDOW_ID);

	lua_pushinteger(p_L, 0);
	return 1;
}

static const luaL_Reg display_funcs[] = {
	{"window_get_size", l_window_get_size},
	{"window_set_size", l_window_set_size},
	{nullptr, nullptr}
};

int luaopen_native_display(lua_State *p_L) {
	luaL_newlib(p_L, display_funcs);
	return 1;
}

} // namespace luagd
