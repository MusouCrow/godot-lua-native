#include "transform_module.h"

#include "node_module.h"

#include <godot_cpp/classes/control.hpp>
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

// 尝试解析为 Node3D。
// 返回：成功返回 Node3D 指针；句柄为空或类型不符时返回 nullptr（不输出错误）。
static godot::Node3D *_try_resolve_node3d(godot::ObjectID p_id) {
	if (p_id.is_null()) {
		return nullptr;
	}

	godot::Node *node = node_resolve_any(p_id);
	if (node == nullptr) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::Node3D>(node);
}

// 尝试解析为 Control。
// 返回：成功返回 Control 指针；句柄为空或类型不符时返回 nullptr（不输出错误）。
static godot::Control *_try_resolve_control(godot::ObjectID p_id) {
	if (p_id.is_null()) {
		return nullptr;
	}

	godot::Node *node = node_resolve_any(p_id);
	if (node == nullptr) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::Control>(node);
}

// set_position(node_id, x, y, z?, is_global?) -> void
// 设置节点位置。
// Node3D: 需要 x, y, z 三个参数，第 5 个参数为 is_global。
// Control: 需要 x, y 两个参数，第 3 或第 4 个参数为 is_global。
static int l_set_position(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);

	// 尝试 Node3D
	godot::Node3D *node3d = _try_resolve_node3d(node_id);
	if (node3d != nullptr) {
		const double x = luaL_checknumber(p_L, 2);
		const double y = luaL_checknumber(p_L, 3);
		const double z = luaL_checknumber(p_L, 4);
		const bool is_global = lua_toboolean(p_L, 5);

		const godot::Vector3 position((float)x, (float)y, (float)z);
		if (is_global) {
			node3d->set_global_position(position);
		} else {
			node3d->set_position(position);
		}
		return 0;
	}

	// 尝试 Control
	godot::Control *control = _try_resolve_control(node_id);
	if (control != nullptr) {
		const double x = luaL_checknumber(p_L, 2);
		const double y = luaL_checknumber(p_L, 3);
		const bool is_global = lua_toboolean(p_L, 4);

		const godot::Vector2 position((float)x, (float)y);
		if (is_global) {
			control->set_global_position(position);
		} else {
			control->set_position(position);
		}
		return 0;
	}

	godot::UtilityFunctions::printerr("native_transform.set_position: node is not Node3D or Control, id=", (uint64_t)node_id);
	return 0;
}

// get_position(node_id, is_global) -> x, y, z | x, y
// 获取节点位置。
// Node3D 返回 3 个值 (x, y, z)，Control 返回 2 个值 (x, y)。
static int l_get_position(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const bool is_global = lua_toboolean(p_L, 2);

	// 尝试 Node3D
	godot::Node3D *node3d = _try_resolve_node3d(node_id);
	if (node3d != nullptr) {
		const godot::Vector3 position = is_global ? node3d->get_global_position() : node3d->get_position();
		lua_pushnumber(p_L, position.x);
		lua_pushnumber(p_L, position.y);
		lua_pushnumber(p_L, position.z);
		return 3;
	}

	// 尝试 Control
	godot::Control *control = _try_resolve_control(node_id);
	if (control != nullptr) {
		const godot::Vector2 position = is_global ? control->get_global_position() : control->get_position();
		lua_pushnumber(p_L, position.x);
		lua_pushnumber(p_L, position.y);
		return 2;
	}

	godot::UtilityFunctions::printerr("native_transform.get_position: node is not Node3D or Control, id=", (uint64_t)node_id);
	return 0;
}

// get_scale(node_id, is_global) -> x, y, z | x, y
// 获取节点缩放。
// Node3D 返回 3 个值 (x, y, z)，支持 is_global。
// Control 返回 2 个值 (x, y)，is_global 参数被忽略（Control 无全局缩放）。
static int l_get_scale(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const bool is_global = lua_toboolean(p_L, 2);

	// 尝试 Node3D
	godot::Node3D *node3d = _try_resolve_node3d(node_id);
	if (node3d != nullptr) {
		const godot::Vector3 scale = is_global ? node3d->get_global_basis().get_scale() : node3d->get_scale();
		lua_pushnumber(p_L, scale.x);
		lua_pushnumber(p_L, scale.y);
		lua_pushnumber(p_L, scale.z);
		return 3;
	}

	// 尝试 Control
	godot::Control *control = _try_resolve_control(node_id);
	if (control != nullptr) {
		const godot::Vector2 scale = control->get_scale();
		lua_pushnumber(p_L, scale.x);
		lua_pushnumber(p_L, scale.y);
		return 2;
	}

	godot::UtilityFunctions::printerr("native_transform.get_scale: node is not Node3D or Control, id=", (uint64_t)node_id);
	return 0;
}

// set_scale(node_id, x, y, z?) -> void
// 设置节点缩放（局部）。
// Node3D 需要 x, y, z 三个参数，Control 需要 x, y 两个参数。
static int l_set_scale(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);

	// 尝试 Node3D
	godot::Node3D *node3d = _try_resolve_node3d(node_id);
	if (node3d != nullptr) {
		const double x = luaL_checknumber(p_L, 2);
		const double y = luaL_checknumber(p_L, 3);
		const double z = luaL_checknumber(p_L, 4);

		const godot::Vector3 scale((float)x, (float)y, (float)z);
		node3d->set_scale(scale);
		return 0;
	}

	// 尝试 Control
	godot::Control *control = _try_resolve_control(node_id);
	if (control != nullptr) {
		const double x = luaL_checknumber(p_L, 2);
		const double y = luaL_checknumber(p_L, 3);

		const godot::Vector2 scale((float)x, (float)y);
		control->set_scale(scale);
		return 0;
	}

	godot::UtilityFunctions::printerr("native_transform.set_scale: node is not Node3D or Control, id=", (uint64_t)node_id);
	return 0;
}

// set_rotation(node_id, x, y, z, is_global) -> void
// 设置节点旋转（度数）。
static int l_set_rotation(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);
	const bool is_global = lua_toboolean(p_L, 5);

	godot::Node3D *node = _try_resolve_node3d(node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_transform.set_rotation: node is not Node3D, id=", (uint64_t)node_id);
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

	godot::Node3D *node = _try_resolve_node3d(node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_transform.get_rotation: node is not Node3D, id=", (uint64_t)node_id);
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

	godot::Node3D *node = _try_resolve_node3d(node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_transform.look_at: node is not Node3D, id=", (uint64_t)node_id);
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

	godot::Node3D *node = _try_resolve_node3d(node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_transform.get_forward: node is not Node3D, id=", (uint64_t)node_id);
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
	{"set_scale", l_set_scale},
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
