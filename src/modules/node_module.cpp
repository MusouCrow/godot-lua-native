#include "node_module.h"

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 节点记录
struct NodeRecord {
	int32_t id;
	godot::CharacterBody3D *node;
};

// 模块级静态数据
static godot::HashMap<int32_t, NodeRecord> nodes;
static int32_t next_id = 1;

// 获取节点记录，不存在时打印错误
static NodeRecord *get_node(int32_t p_id, const char *p_func_name) {
	if (!nodes.has(p_id)) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": invalid id ", p_id);
		return nullptr;
	}
	NodeRecord *rec = &nodes[p_id];

	// 检查节点是否仍有效
	if (rec->node == nullptr || !rec->node->is_inside_tree()) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": node is no longer valid, id ", p_id);
		return nullptr;
	}

	return rec;
}

// ============================================================================
// 节点引用管理
// ============================================================================

// get_by_path(path) -> id
// 通过场景路径获取节点句柄。
// 目前仅支持 CharacterBody3D 节点。
// 返回节点 id，失败返回 -1。
static int l_get_by_path(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(
		godot::Engine::get_singleton()->get_main_loop()
	);
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: SceneTree not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Window *root_window = tree->get_root();
	if (root_window == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: Scene root not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// Window 继承自 Viewport，需要找到实际的根节点
	// get_root() 返回的 Window 就是场景树的根，可以作为节点访问
	godot::Node *root = godot::Object::cast_to<godot::Node>(root_window);
	if (root == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: Scene root is not a Node");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::NodePath node_path((godot::String(path)));
	godot::Node *found_node = root->get_node<godot::Node>(node_path);
	if (found_node == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: node not found: ", path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::CharacterBody3D *body = godot::Object::cast_to<godot::CharacterBody3D>(found_node);
	if (body == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: node is not a CharacterBody3D: ", path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	NodeRecord rec;
	rec.id = next_id++;
	rec.node = body;
	nodes[rec.id] = rec;

	lua_pushinteger(p_L, rec.id);
	return 1;
}

// release(id) -> void
// 释放节点引用。
// 注意：不会销毁节点本身，仅释放模块内的引用。
static int l_release(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	if (!nodes.has(id)) {
		// 重复释放不报错，静默处理
		return 0;
	}

	nodes.erase(id);
	return 0;
}

// is_valid(id) -> bool
// 检查节点引用是否有效。
static int l_is_valid(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	if (!nodes.has(id)) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	NodeRecord *rec = &nodes[id];
	bool valid = rec->node != nullptr && rec->node->is_inside_tree();
	lua_pushboolean(p_L, valid);
	return 1;
}

// ============================================================================
// 位置
// ============================================================================

// set_position(id, x, y, z, is_global) -> void
// 设置节点位置。
// is_global: true 为世界坐标，false 为局部坐标（默认）。
static int l_set_position(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool is_global = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "set_position");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 pos((float)x, (float)y, (float)z);
	if (is_global) {
		rec->node->set_global_position(pos);
	} else {
		rec->node->set_position(pos);
	}
	return 0;
}

// get_position(id, is_global) -> x, y, z
// 获取节点位置。
// is_global: true 为世界坐标，false 为局部坐标（默认）。
static int l_get_position(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);

	NodeRecord *rec = get_node(id, "get_position");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 pos = is_global ? rec->node->get_global_position()
	                                : rec->node->get_position();
	lua_pushnumber(p_L, pos.x);
	lua_pushnumber(p_L, pos.y);
	lua_pushnumber(p_L, pos.z);
	return 3;
}

// ============================================================================
// 旋转
// ============================================================================

// set_rotation(id, x, y, z, is_global) -> void
// 设置节点旋转（弧度）。
// is_global: true 为世界旋转，false 为局部旋转（默认）。
static int l_set_rotation(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool is_global = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "set_rotation");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 rot((float)x, (float)y, (float)z);
	if (is_global) {
		rec->node->set_global_rotation(rot);
	} else {
		rec->node->set_rotation(rot);
	}
	return 0;
}

// get_rotation(id, is_global) -> x, y, z
// 获取节点旋转（弧度）。
// is_global: true 为世界旋转，false 为局部旋转（默认）。
static int l_get_rotation(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);

	NodeRecord *rec = get_node(id, "get_rotation");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 rot = is_global ? rec->node->get_global_rotation()
	                                : rec->node->get_rotation();
	lua_pushnumber(p_L, rot.x);
	lua_pushnumber(p_L, rot.y);
	lua_pushnumber(p_L, rot.z);
	return 3;
}

// look_at(id, target_x, target_y, target_z) -> void
// 使节点朝向目标位置。
// 节点的 -Z 轴（前向）将指向目标位置。
static int l_look_at(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);

	NodeRecord *rec = get_node(id, "look_at");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 target((float)x, (float)y, (float)z);
	rec->node->look_at(target);
	return 0;
}

// ============================================================================
// 移动
// ============================================================================

// move_and_slide(id) -> bool
// 执行移动并滑动。
// 应在 physics_process 中调用。
// 返回是否发生碰撞。
static int l_move_and_slide(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "move_and_slide");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	bool collided = rec->node->move_and_slide();
	lua_pushboolean(p_L, collided);
	return 1;
}

// set_velocity(id, x, y, z) -> void
// 设置节点的速度向量。
// 注意：不要乘以 delta，move_and_slide 会自动处理。
static int l_set_velocity(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);

	NodeRecord *rec = get_node(id, "set_velocity");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 vel((float)x, (float)y, (float)z);
	rec->node->set_velocity(vel);
	return 0;
}

// get_velocity(id) -> x, y, z
// 获取节点的速度向量。
static int l_get_velocity(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "get_velocity");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 vel = rec->node->get_velocity();
	lua_pushnumber(p_L, vel.x);
	lua_pushnumber(p_L, vel.y);
	lua_pushnumber(p_L, vel.z);
	return 3;
}

// get_real_velocity(id) -> x, y, z
// 获取节点的实际移动速度。
// 考虑滑动后的实际速度。
static int l_get_real_velocity(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "get_real_velocity");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 vel = rec->node->get_real_velocity();
	lua_pushnumber(p_L, vel.x);
	lua_pushnumber(p_L, vel.y);
	lua_pushnumber(p_L, vel.z);
	return 3;
}

// is_on_floor(id) -> bool
// 检查节点是否在地面上。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_floor(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "is_on_floor");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, rec->node->is_on_floor());
	return 1;
}

// is_on_wall(id) -> bool
// 检查节点是否在墙上。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_wall(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "is_on_wall");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, rec->node->is_on_wall());
	return 1;
}

// is_on_ceiling(id) -> bool
// 检查节点是否在天花板上。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_ceiling(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "is_on_ceiling");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, rec->node->is_on_ceiling());
	return 1;
}

// get_floor_normal(id) -> x, y, z
// 获取地面的碰撞法线。
// 仅在 move_and_slide 调用后且 is_on_floor 为 true 时有效。
static int l_get_floor_normal(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "get_floor_normal");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 normal = rec->node->get_floor_normal();
	lua_pushnumber(p_L, normal.x);
	lua_pushnumber(p_L, normal.y);
	lua_pushnumber(p_L, normal.z);
	return 3;
}

// ============================================================================
// 信息
// ============================================================================

// get_name(id) -> string
// 获取节点名称。
static int l_get_name(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "get_name");
	if (rec == nullptr) {
		return 0;
	}

	godot::String name = rec->node->get_name();
	godot::CharString utf8_name = name.utf8();
	lua_pushstring(p_L, utf8_name.get_data());
	return 1;
}

// ============================================================================
// 模块注册
// ============================================================================

static const luaL_Reg node_funcs[] = {
	// 节点引用管理
	{"get_by_path", l_get_by_path},
	{"release", l_release},
	{"is_valid", l_is_valid},

	// 位置
	{"set_position", l_set_position},
	{"get_position", l_get_position},

	// 旋转
	{"set_rotation", l_set_rotation},
	{"get_rotation", l_get_rotation},
	{"look_at", l_look_at},

	// 移动
	{"move_and_slide", l_move_and_slide},
	{"set_velocity", l_set_velocity},
	{"get_velocity", l_get_velocity},
	{"get_real_velocity", l_get_real_velocity},
	{"is_on_floor", l_is_on_floor},
	{"is_on_wall", l_is_on_wall},
	{"is_on_ceiling", l_is_on_ceiling},
	{"get_floor_normal", l_get_floor_normal},

	// 信息
	{"get_name", l_get_name},

	{nullptr, nullptr}
};

int luaopen_native_node(lua_State *p_L) {
	luaL_newlib(p_L, node_funcs);
	return 1;
}

void node_cleanup() {
	// 清除所有节点引用
	// 注意：不销毁节点本身，节点由 Godot 场景树管理
	nodes.clear();
	next_id = 1;
}

} // namespace luagd
