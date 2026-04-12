#include "debug_draw_module.h"

#include "../debug_draw/debug_draw_types.h"

#include "../host/host_thread_check.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 模块级状态。
// 延迟初始化，避免静态 Ref 在扩展装载阶段触发未定义行为。
static DebugDrawState *debug_draw_state = nullptr;

// 返回调试绘制模块状态。
// 注意：首次调用时会完成延迟初始化。
DebugDrawState &debug_draw_get_state() {
	if (debug_draw_state == nullptr) {
		debug_draw_state = memnew(DebugDrawState);
	}
	return *debug_draw_state;
}

// 清空本帧命令缓存。
void debug_draw_clear_commands() {
	DebugDrawState &state = debug_draw_get_state();
	state.point_commands.clear();
	state.line_commands.clear();
	state.circle_commands.clear();
	state.sector_commands.clear();
	state.scratch.clear();
}

// 统一处理 native_debug_draw 的主线程约束。
static bool _ensure_main_thread(const char *p_func_name) {
	return ensure_main_thread(godot::String("native_debug_draw.") + p_func_name);
}

// 从连续 4 个 Lua number 读取 Color。
static godot::Color _make_color(lua_State *p_L, int p_index) {
	return godot::Color(
		(float)luaL_checknumber(p_L, p_index),
		(float)luaL_checknumber(p_L, p_index + 1),
		(float)luaL_checknumber(p_L, p_index + 2),
		(float)luaL_checknumber(p_L, p_index + 3)
	);
}

// 解析并绑定调试绘制根节点。
// 约束：路径必须指向场景树中的 Node3D。
static bool _set_root_path(const char *p_path) {
	DebugDrawState &state = debug_draw_get_state();

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(godot::Engine::get_singleton()->get_main_loop());
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_debug_draw.set_root: SceneTree not available");
		return false;
	}

	godot::Window *root_window = tree->get_root();
	if (root_window == nullptr) {
		godot::UtilityFunctions::printerr("native_debug_draw.set_root: scene root not available");
		return false;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_window);
	if (root_node == nullptr) {
		godot::UtilityFunctions::printerr("native_debug_draw.set_root: root window is not a Node");
		return false;
	}

	godot::Node *found_node = root_node->get_node_or_null(godot::NodePath(godot::String(p_path)));
	if (found_node == nullptr) {
		godot::UtilityFunctions::printerr("native_debug_draw.set_root: node not found: ", p_path);
		return false;
	}

	godot::Node3D *found_node3d = godot::Object::cast_to<godot::Node3D>(found_node);
	if (found_node3d == nullptr) {
		godot::UtilityFunctions::printerr("native_debug_draw.set_root: node is not Node3D: ", p_path);
		return false;
	}

	if (state.root_node_id != godot::ObjectID(found_node3d->get_instance_id())) {
		debug_draw_reset_scene(true);
		debug_draw_clear_commands();
	}

	state.root_node_id = godot::ObjectID(found_node3d->get_instance_id());
	return debug_draw_ensure_scene();
}

// set_root(path) -> bool
// 设置调试绘制根节点。
// 返回：绑定成功返回 true，失败返回 false。
static int l_set_root(lua_State *p_L) {
	if (!_ensure_main_thread("set_root")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const char *path = luaL_checkstring(p_L, 1);
	const bool ok = _set_root_path(path);
	lua_pushboolean(p_L, ok);
	return 1;
}

// set_enabled(enabled) -> bool
// 设置模块是否启用。
// 返回：处理成功返回 true，主线程检查失败返回 false。
static int l_set_enabled(lua_State *p_L) {
	if (!_ensure_main_thread("set_enabled")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	DebugDrawState &state = debug_draw_get_state();
	state.enabled = lua_toboolean(p_L, 1);
	if (!state.enabled) {
		debug_draw_clear_commands();
		debug_draw_clear_meshes();
	}
	debug_draw_update_visibility();
	lua_pushboolean(p_L, true);
	return 1;
}

// is_enabled() -> bool
// 查询模块是否启用。
static int l_is_enabled(lua_State *p_L) {
	if (!_ensure_main_thread("is_enabled")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, debug_draw_get_state().enabled);
	return 1;
}

// set_visible(visible) -> bool
// 设置调试绘制节点可见性。
// 返回：处理成功返回 true，主线程检查失败返回 false。
static int l_set_visible(lua_State *p_L) {
	if (!_ensure_main_thread("set_visible")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	DebugDrawState &state = debug_draw_get_state();
	state.visible = lua_toboolean(p_L, 1);
	debug_draw_update_visibility();
	lua_pushboolean(p_L, true);
	return 1;
}

// clear() -> void
// 清空命令缓存和已显示 mesh。
static int l_clear(lua_State *p_L) {
	if (!_ensure_main_thread("clear")) {
		return 0;
	}

	debug_draw_clear_commands();
	debug_draw_clear_meshes();
	return 0;
}

// submit() -> bool
// 提交当前缓存的调试绘制命令。
// 返回：提交成功返回 true；未启用或提交失败返回 false。
static int l_submit(lua_State *p_L) {
	if (!_ensure_main_thread("submit")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (!debug_draw_get_state().enabled) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const bool ok = debug_draw_build_and_submit();
	lua_pushboolean(p_L, ok);
	return 1;
}

// add_point(x, y, z, size, r, g, b, a, is_xray) -> bool
// 添加调试点命令。
// 返回：参数合法返回 true，否则返回 false。
static int l_add_point(lua_State *p_L) {
	if (!_ensure_main_thread("add_point")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	DebugPointCommand command;
	command.position = godot::Vector3(
		(float)luaL_checknumber(p_L, 1),
		(float)luaL_checknumber(p_L, 2),
		(float)luaL_checknumber(p_L, 3)
	);
	command.size = (float)luaL_checknumber(p_L, 4);
	command.color = _make_color(p_L, 5);
	command.is_xray = lua_toboolean(p_L, 9);

	if (command.size <= 0.0f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_point: size must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}

	debug_draw_get_state().point_commands.push_back(command);
	lua_pushboolean(p_L, true);
	return 1;
}

// add_line(from_x, from_y, from_z, to_x, to_y, to_z, width, r, g, b, a, is_xray) -> bool
// 添加调试线命令。
// 返回：参数合法返回 true，否则返回 false。
static int l_add_line(lua_State *p_L) {
	if (!_ensure_main_thread("add_line")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	DebugLineCommand command;
	command.from = godot::Vector3(
		(float)luaL_checknumber(p_L, 1),
		(float)luaL_checknumber(p_L, 2),
		(float)luaL_checknumber(p_L, 3)
	);
	command.to = godot::Vector3(
		(float)luaL_checknumber(p_L, 4),
		(float)luaL_checknumber(p_L, 5),
		(float)luaL_checknumber(p_L, 6)
	);
	command.width = (float)luaL_checknumber(p_L, 7);
	command.color = _make_color(p_L, 8);
	command.is_xray = lua_toboolean(p_L, 12);

	if (command.width <= 0.0f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_line: width must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}
	if ((command.to - command.from).length_squared() <= 0.000001f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_line: line length must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}

	debug_draw_get_state().line_commands.push_back(command);
	lua_pushboolean(p_L, true);
	return 1;
}

// add_circle(center_x, center_y, center_z, normal_x, normal_y, normal_z, radius, segments, line_width, r, g, b, a, is_xray, is_fill) -> bool
// 添加调试圆命令。
// 注意：is_fill=true 表示实心圆面，不表示圆环。
static int l_add_circle(lua_State *p_L) {
	if (!_ensure_main_thread("add_circle")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	DebugCircleCommand command;
	command.center = godot::Vector3(
		(float)luaL_checknumber(p_L, 1),
		(float)luaL_checknumber(p_L, 2),
		(float)luaL_checknumber(p_L, 3)
	);
	command.normal = godot::Vector3(
		(float)luaL_checknumber(p_L, 4),
		(float)luaL_checknumber(p_L, 5),
		(float)luaL_checknumber(p_L, 6)
	);
	command.radius = (float)luaL_checknumber(p_L, 7);
	command.segments = (int32_t)luaL_checkinteger(p_L, 8);
	command.line_width = (float)luaL_checknumber(p_L, 9);
	command.color = _make_color(p_L, 10);
	command.is_xray = lua_toboolean(p_L, 14);
	command.is_fill = lua_toboolean(p_L, 15);

	if (command.radius <= 0.0f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_circle: radius must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}
	if (command.normal.length_squared() <= 0.000001f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_circle: normal length must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}
	if (command.segments < 3) {
		command.segments = 3;
	}
	if (!command.is_fill && command.line_width <= 0.0f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_circle: line_width must be > 0 when is_fill is false");
		lua_pushboolean(p_L, false);
		return 1;
	}

	debug_draw_get_state().circle_commands.push_back(command);
	lua_pushboolean(p_L, true);
	return 1;
}

// add_sector(center_x, center_y, center_z, normal_x, normal_y, normal_z, dir_x, dir_y, dir_z, radius, angle_degrees, segments, line_width, r, g, b, a, is_xray, is_fill) -> bool
// 添加调试扇形命令。
// 返回：参数合法返回 true，否则返回 false。
static int l_add_sector(lua_State *p_L) {
	if (!_ensure_main_thread("add_sector")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	DebugSectorCommand command;
	command.center = godot::Vector3(
		(float)luaL_checknumber(p_L, 1),
		(float)luaL_checknumber(p_L, 2),
		(float)luaL_checknumber(p_L, 3)
	);
	command.normal = godot::Vector3(
		(float)luaL_checknumber(p_L, 4),
		(float)luaL_checknumber(p_L, 5),
		(float)luaL_checknumber(p_L, 6)
	);
	command.direction = godot::Vector3(
		(float)luaL_checknumber(p_L, 7),
		(float)luaL_checknumber(p_L, 8),
		(float)luaL_checknumber(p_L, 9)
	);
	command.radius = (float)luaL_checknumber(p_L, 10);
	command.angle_degrees = (float)luaL_checknumber(p_L, 11);
	command.segments = (int32_t)luaL_checkinteger(p_L, 12);
	command.line_width = (float)luaL_checknumber(p_L, 13);
	command.color = _make_color(p_L, 14);
	command.is_xray = lua_toboolean(p_L, 18);
	command.is_fill = lua_toboolean(p_L, 19);

	if (command.radius <= 0.0f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_sector: radius must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}
	if (command.normal.length_squared() <= 0.000001f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_sector: normal length must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}
	if (command.direction.length_squared() <= 0.000001f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_sector: direction length must be > 0");
		lua_pushboolean(p_L, false);
		return 1;
	}
	if (command.angle_degrees <= 0.0f || command.angle_degrees > 360.0f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_sector: angle_degrees must be in (0, 360]");
		lua_pushboolean(p_L, false);
		return 1;
	}
	if (command.segments < 1) {
		command.segments = 1;
	}
	if (!command.is_fill && command.line_width <= 0.0f) {
		godot::UtilityFunctions::printerr("native_debug_draw.add_sector: line_width must be > 0 when is_fill is false");
		lua_pushboolean(p_L, false);
		return 1;
	}

	debug_draw_get_state().sector_commands.push_back(command);
	lua_pushboolean(p_L, true);
	return 1;
}

// get_stats() -> point_commands, line_commands, circle_commands, sector_commands, points_vertex_count, lines_vertex_count, faces_vertex_count, submit_count, had_camera
// 返回最近一次 submit 的统计信息。
static int l_get_stats(lua_State *p_L) {
	if (!_ensure_main_thread("get_stats")) {
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushinteger(p_L, 0);
		lua_pushboolean(p_L, false);
		return 9;
	}

	const DebugFrameStats &stats = debug_draw_get_state().stats;
	lua_pushinteger(p_L, stats.point_commands);
	lua_pushinteger(p_L, stats.line_commands);
	lua_pushinteger(p_L, stats.circle_commands);
	lua_pushinteger(p_L, stats.sector_commands);
	lua_pushinteger(p_L, stats.points_vertex_count);
	lua_pushinteger(p_L, stats.lines_vertex_count);
	lua_pushinteger(p_L, stats.faces_vertex_count);
	lua_pushinteger(p_L, stats.submit_count);
	lua_pushboolean(p_L, stats.had_camera);
	return 9;
}

static const luaL_Reg debug_draw_funcs[] = {
	{"set_root", l_set_root},
	{"set_enabled", l_set_enabled},
	{"is_enabled", l_is_enabled},
	{"set_visible", l_set_visible},
	{"clear", l_clear},
	{"submit", l_submit},
	{"add_point", l_add_point},
	{"add_line", l_add_line},
	{"add_circle", l_add_circle},
	{"add_sector", l_add_sector},
	{"get_stats", l_get_stats},
	{nullptr, nullptr}
};

int luaopen_native_debug_draw(lua_State *p_L) {
	luaL_newlib(p_L, debug_draw_funcs);
	return 1;
}

// 清理模块状态。
// 注意：场景对象交给引擎统一销毁，这里只清理模块记录。
void debug_draw_cleanup() {
	if (debug_draw_state == nullptr) {
		return;
	}

	debug_draw_clear_commands();
	debug_draw_clear_meshes();
	debug_draw_reset_scene(false);
	debug_draw_state->root_node_id = godot::ObjectID();
	debug_draw_state->enabled = true;
	debug_draw_state->visible = true;
	debug_draw_state->stats.reset();
	godot::memdelete(debug_draw_state);
	debug_draw_state = nullptr;
}

} // namespace luagd
