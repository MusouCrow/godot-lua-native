#include "transform_module.h"

#include "node_module.h"

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

static godot::ObjectID _read_node_id(lua_State *p_L, int p_index) {
	return godot::ObjectID((uint64_t)luaL_checkinteger(p_L, p_index));
}

static godot::Node3D *_resolve_node(godot::ObjectID p_node_id, const char *p_func_name) {
	if (p_node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_transform.", p_func_name, ": node id is 0");
		return nullptr;
	}

	godot::Node3D *node = node_resolve(p_node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_transform.", p_func_name, ": node is no longer valid, id ", p_node_id);
		return nullptr;
	}

	return node;
}

// set_position(node_id, x, y, z, is_global) -> void
// 设置节点位置。
static int l_set_position(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);
	const bool is_global = lua_toboolean(p_L, 5);

	godot::Node3D *node = _resolve_node(node_id, "set_position");
	if (node == nullptr) {
		return 0;
	}

	const godot::Vector3 position((float)x, (float)y, (float)z);
	if (is_global) {
		node->set_global_position(position);
	} else {
		node->set_position(position);
	}
	return 0;
}

// get_position(node_id, is_global) -> x, y, z
// 获取节点位置。
static int l_get_position(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const bool is_global = lua_toboolean(p_L, 2);

	godot::Node3D *node = _resolve_node(node_id, "get_position");
	if (node == nullptr) {
		return 0;
	}

	const godot::Vector3 position = is_global ? node->get_global_position() : node->get_position();
	lua_pushnumber(p_L, position.x);
	lua_pushnumber(p_L, position.y);
	lua_pushnumber(p_L, position.z);
	return 3;
}

// get_scale(node_id, is_global) -> x, y, z
// 获取节点缩放。
static int l_get_scale(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const bool is_global = lua_toboolean(p_L, 2);

	godot::Node3D *node = _resolve_node(node_id, "get_scale");
	if (node == nullptr) {
		return 0;
	}

	const godot::Vector3 scale = is_global ? node->get_global_basis().get_scale() : node->get_scale();
	lua_pushnumber(p_L, scale.x);
	lua_pushnumber(p_L, scale.y);
	lua_pushnumber(p_L, scale.z);
	return 3;
}

// set_rotation(node_id, x, y, z, is_global) -> void
// 设置节点旋转（度数）。
static int l_set_rotation(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);
	const bool is_global = lua_toboolean(p_L, 5);

	godot::Node3D *node = _resolve_node(node_id, "set_rotation");
	if (node == nullptr) {
		return 0;
	}

	const godot::Vector3 rotation((float)x, (float)y, (float)z);
	if (is_global) {
		node->set_global_rotation_degrees(rotation);
	} else {
		node->set_rotation_degrees(rotation);
	}
	return 0;
}

// get_rotation(node_id, is_global) -> x, y, z
// 获取节点旋转（度数）。
static int l_get_rotation(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const bool is_global = lua_toboolean(p_L, 2);

	godot::Node3D *node = _resolve_node(node_id, "get_rotation");
	if (node == nullptr) {
		return 0;
	}

	const godot::Vector3 rotation = is_global ? node->get_global_rotation_degrees() : node->get_rotation_degrees();
	lua_pushnumber(p_L, rotation.x);
	lua_pushnumber(p_L, rotation.y);
	lua_pushnumber(p_L, rotation.z);
	return 3;
}

// look_at(node_id, target_x, target_y, target_z, use_model_front) -> void
// 使节点朝向目标位置。
static int l_look_at(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);
	const bool use_model_front = lua_toboolean(p_L, 5);

	godot::Node3D *node = _resolve_node(node_id, "look_at");
	if (node == nullptr) {
		return 0;
	}

	const godot::Vector3 target((float)x, (float)y, (float)z);
	node->look_at(target, godot::Vector3(0.0f, 1.0f, 0.0f), use_model_front);
	return 0;
}

// get_forward(node_id, is_global, use_model_front) -> x, y, z
// 获取节点前向向量。
static int l_get_forward(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const bool is_global = lua_toboolean(p_L, 2);
	const bool use_model_front = lua_toboolean(p_L, 3);

	godot::Node3D *node = _resolve_node(node_id, "get_forward");
	if (node == nullptr) {
		return 0;
	}

	const godot::Basis basis = is_global ? node->get_global_transform().basis : node->get_transform().basis;
	const godot::Vector3 forward = use_model_front ? basis.get_column(2) : -basis.get_column(2);
	lua_pushnumber(p_L, forward.x);
	lua_pushnumber(p_L, forward.y);
	lua_pushnumber(p_L, forward.z);
	return 3;
}

static const luaL_Reg transform_funcs[] = {
	{"set_position", l_set_position},
	{"get_position", l_get_position},
	{"get_scale", l_get_scale},
	{"set_rotation", l_set_rotation},
	{"get_rotation", l_get_rotation},
	{"look_at", l_look_at},
	{"get_forward", l_get_forward},
	{nullptr, nullptr}
};

int luaopen_native_transform(lua_State *p_L) {
	luaL_newlib(p_L, transform_funcs);
	return 1;
}

} // namespace luagd
