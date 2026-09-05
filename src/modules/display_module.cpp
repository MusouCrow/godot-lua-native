#include "display_module.h"

#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
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

// native_display.window_get_mode() -> mode:int
// 返回：当前窗口模式（见 WINDOW_MODE_* 常量）；DisplayServer 不可用时返回 -1。
static int l_window_get_mode(lua_State *p_L) {
	godot::DisplayServer *ds = godot::DisplayServer::get_singleton();
	if (ds == nullptr) {
		godot::UtilityFunctions::printerr("native_display.window_get_mode: DisplayServer not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::DisplayServer::WindowMode mode = ds->window_get_mode(MAIN_WINDOW_ID);
	lua_pushinteger(p_L, (int32_t)mode);
	return 1;
}

// native_display.window_set_mode(mode:int) -> rc:int
// 返回：成功返回 0，失败返回 -1。
// 约束：mode 必须为合法窗口模式（见 WINDOW_MODE_* 常量）；DisplayServer 不可用时返回 -1。
static int l_window_set_mode(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_display.window_set_mode: expected 1 argument (mode), got ", argc);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	if (!lua_isinteger(p_L, 1)) {
		godot::UtilityFunctions::printerr("native_display.window_set_mode: argument must be an integer");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	int64_t mode = lua_tointeger(p_L, 1);
	if (mode < godot::DisplayServer::WINDOW_MODE_WINDOWED ||
		mode > godot::DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN) {
		godot::UtilityFunctions::printerr("native_display.window_set_mode: invalid mode ", mode);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::DisplayServer *ds = godot::DisplayServer::get_singleton();
	if (ds == nullptr) {
		godot::UtilityFunctions::printerr("native_display.window_set_mode: DisplayServer not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	ds->window_set_mode((godot::DisplayServer::WindowMode)mode, MAIN_WINDOW_ID);

	lua_pushinteger(p_L, 0);
	return 1;
}

// 向模块表写入一个枚举常量。
static void l_register_constant(lua_State *p_L, const char *p_name, int32_t p_value) {
	lua_pushinteger(p_L, p_value);
	lua_setfield(p_L, -2, p_name);
}

// native_display.window_set_center_position() -> rc:int
// 返回：成功返回 0，失败返回 -1。
// 约束：仅窗口模式有效；DisplayServer 不可用时返回 -1。
// 将主窗口居中到当前屏幕中心，用于从全屏切换到窗口后恢复坐标。
static int l_window_set_center_position(lua_State *p_L) {
	godot::DisplayServer *ds = godot::DisplayServer::get_singleton();
	if (ds == nullptr) {
		godot::UtilityFunctions::printerr("native_display.window_set_center_position: DisplayServer not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	int32_t screen = ds->window_get_current_screen(MAIN_WINDOW_ID);
	godot::Vector2i screen_size = ds->screen_get_size(screen);
	godot::Vector2i window_size = ds->window_get_size(MAIN_WINDOW_ID);

	godot::Vector2i center = (screen_size - window_size) / 2;
	ds->window_set_position(center, MAIN_WINDOW_ID);

	lua_pushinteger(p_L, 0);
	return 1;
}

// native_display.content_scale_set_factor(scale:number) -> rc:int
// 返回：成功返回 0，失败返回 -1。
// 约束：scale 必须为正数；根窗口不可用时返回 -1。
// 设置内容缩放因子，对应项目设置 display/window/stretch/scale。
static int l_content_scale_set_factor(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_display.content_scale_set_factor: expected 1 argument (scale), got ", argc);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	double scale = luaL_checknumber(p_L, 1);
	if (scale <= 0.0) {
		godot::UtilityFunctions::printerr("native_display.content_scale_set_factor: scale must be > 0");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(
			godot::Engine::get_singleton()->get_main_loop());
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_display.content_scale_set_factor: SceneTree not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Window *root = tree->get_root();
	root->set_content_scale_factor((float)scale);

	lua_pushinteger(p_L, 0);
	return 1;
}

static const luaL_Reg display_funcs[] = {
	{"window_get_size", l_window_get_size},
	{"window_set_size", l_window_set_size},
	{"window_get_mode", l_window_get_mode},
	{"window_set_mode", l_window_set_mode},
	{"window_set_center_position", l_window_set_center_position},
	{"content_scale_set_factor", l_content_scale_set_factor},
	{nullptr, nullptr}
};

int luaopen_native_display(lua_State *p_L) {
	luaL_newlib(p_L, display_funcs);
	l_register_constant(p_L, "WINDOW_MODE_WINDOWED", godot::DisplayServer::WINDOW_MODE_WINDOWED);
	l_register_constant(p_L, "WINDOW_MODE_MINIMIZED", godot::DisplayServer::WINDOW_MODE_MINIMIZED);
	l_register_constant(p_L, "WINDOW_MODE_MAXIMIZED", godot::DisplayServer::WINDOW_MODE_MAXIMIZED);
	l_register_constant(p_L, "WINDOW_MODE_FULLSCREEN", godot::DisplayServer::WINDOW_MODE_FULLSCREEN);
	l_register_constant(p_L, "WINDOW_MODE_EXCLUSIVE_FULLSCREEN", godot::DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN);
	return 1;
}

} // namespace luagd