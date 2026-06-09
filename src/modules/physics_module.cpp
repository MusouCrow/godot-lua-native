#include "physics_module.h"

#include "node_module.h"

#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/core/object.hpp>
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
		godot::UtilityFunctions::printerr("native_physics.", p_func_name, ": node id is 0");
		return nullptr;
	}

	godot::Node3D *node = node_resolve(p_node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_physics.", p_func_name, ": node is no longer valid, id ", p_node_id);
		return nullptr;
	}

	return node;
}

static godot::CharacterBody3D *_resolve_character_body(godot::ObjectID p_node_id, const char *p_func_name) {
	godot::Node3D *node = _resolve_node(p_node_id, p_func_name);
	if (node == nullptr) {
		return nullptr;
	}

	godot::CharacterBody3D *body = godot::Object::cast_to<godot::CharacterBody3D>(node);
	if (body == nullptr) {
		godot::UtilityFunctions::printerr("native_physics.", p_func_name, ": node is not CharacterBody3D, id ", p_node_id);
		return nullptr;
	}

	return body;
}

static godot::CollisionObject3D *_resolve_collision_object(godot::ObjectID p_node_id, const char *p_func_name) {
	godot::Node3D *node = _resolve_node(p_node_id, p_func_name);
	if (node == nullptr) {
		return nullptr;
	}

	godot::CollisionObject3D *collision_object = godot::Object::cast_to<godot::CollisionObject3D>(node);
	if (collision_object == nullptr) {
		godot::UtilityFunctions::printerr("native_physics.", p_func_name, ": node is not CollisionObject3D, id ", p_node_id);
		return nullptr;
	}

	return collision_object;
}

// move_and_slide(node_id) -> bool
// 执行移动并滑动。
static int l_move_and_slide(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "move_and_slide");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->move_and_slide());
	return 1;
}

// set_velocity(node_id, x, y, z) -> void
// 设置速度向量。
static int l_set_velocity(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);

	godot::CharacterBody3D *body = _resolve_character_body(node_id, "set_velocity");
	if (body == nullptr) {
		return 0;
	}

	body->set_velocity(godot::Vector3((float)x, (float)y, (float)z));
	return 0;
}

// get_velocity(node_id) -> x, y, z
// 获取速度向量。
static int l_get_velocity(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "get_velocity");
	if (body == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	const godot::Vector3 velocity = body->get_velocity();
	lua_pushnumber(p_L, velocity.x);
	lua_pushnumber(p_L, velocity.y);
	lua_pushnumber(p_L, velocity.z);
	return 3;
}

// get_real_velocity(node_id) -> x, y, z
// 获取实际移动速度。
static int l_get_real_velocity(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "get_real_velocity");
	if (body == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	const godot::Vector3 velocity = body->get_real_velocity();
	lua_pushnumber(p_L, velocity.x);
	lua_pushnumber(p_L, velocity.y);
	lua_pushnumber(p_L, velocity.z);
	return 3;
}

// is_on_floor(node_id) -> bool
// 检查是否在地面上。
static int l_is_on_floor(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "is_on_floor");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->is_on_floor());
	return 1;
}

// is_on_wall(node_id) -> bool
// 检查是否在墙上。
static int l_is_on_wall(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "is_on_wall");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->is_on_wall());
	return 1;
}

// is_on_ceiling(node_id) -> bool
// 检查是否在天花板上。
static int l_is_on_ceiling(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "is_on_ceiling");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->is_on_ceiling());
	return 1;
}

// get_floor_normal(node_id) -> x, y, z
// 获取地面法线。
static int l_get_floor_normal(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "get_floor_normal");
	if (body == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	const godot::Vector3 normal = body->get_floor_normal();
	lua_pushnumber(p_L, normal.x);
	lua_pushnumber(p_L, normal.y);
	lua_pushnumber(p_L, normal.z);
	return 3;
}

// set_collision_layer(node_id, layer) -> void
// 设置 CollisionObject3D 的碰撞层。
static int l_set_collision_layer(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const uint32_t layer = (uint32_t)luaL_checkinteger(p_L, 2);

	godot::CollisionObject3D *collision_object = _resolve_collision_object(node_id, "set_collision_layer");
	if (collision_object == nullptr) {
		return 0;
	}

	collision_object->set_collision_layer(layer);
	return 0;
}

// get_collision_layer(node_id) -> integer
// 获取 CollisionObject3D 的碰撞层。
static int l_get_collision_layer(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CollisionObject3D *collision_object = _resolve_collision_object(node_id, "get_collision_layer");
	if (collision_object == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	lua_pushinteger(p_L, collision_object->get_collision_layer());
	return 1;
}

// set_collision_mask(node_id, mask) -> void
// 设置 CollisionObject3D 的碰撞掩码。
static int l_set_collision_mask(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const uint32_t mask = (uint32_t)luaL_checkinteger(p_L, 2);

	godot::CollisionObject3D *collision_object = _resolve_collision_object(node_id, "set_collision_mask");
	if (collision_object == nullptr) {
		return 0;
	}

	collision_object->set_collision_mask(mask);
	return 0;
}

// get_collision_mask(node_id) -> integer
// 获取 CollisionObject3D 的碰撞掩码。
static int l_get_collision_mask(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CollisionObject3D *collision_object = _resolve_collision_object(node_id, "get_collision_mask");
	if (collision_object == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	lua_pushinteger(p_L, collision_object->get_collision_mask());
	return 1;
}

static const luaL_Reg physics_funcs[] = {
	{"move_and_slide", l_move_and_slide},
	{"set_velocity", l_set_velocity},
	{"get_velocity", l_get_velocity},
	{"get_real_velocity", l_get_real_velocity},
	{"is_on_floor", l_is_on_floor},
	{"is_on_wall", l_is_on_wall},
	{"is_on_ceiling", l_is_on_ceiling},
	{"get_floor_normal", l_get_floor_normal},
	{"set_collision_layer", l_set_collision_layer},
	{"get_collision_layer", l_get_collision_layer},
	{"set_collision_mask", l_set_collision_mask},
	{"get_collision_mask", l_get_collision_mask},
	{nullptr, nullptr}
};

int luaopen_native_physics(lua_State *p_L) {
	luaL_newlib(p_L, physics_funcs);
	return 1;
}

} // namespace luagd
