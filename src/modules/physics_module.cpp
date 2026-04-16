#include "physics_module.h"

#include "node_module.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>

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

static int _push_aabb(lua_State *p_L, const godot::AABB &p_aabb) {
	lua_pushnumber(p_L, p_aabb.position.x);
	lua_pushnumber(p_L, p_aabb.position.y);
	lua_pushnumber(p_L, p_aabb.position.z);
	lua_pushnumber(p_L, p_aabb.size.x);
	lua_pushnumber(p_L, p_aabb.size.y);
	lua_pushnumber(p_L, p_aabb.size.z);
	return 6;
}

static int _push_zero_aabb(lua_State *p_L) {
	return _push_aabb(p_L, godot::AABB());
}

static bool _try_get_shape_local_aabb(const godot::Ref<godot::Shape3D> &p_shape, godot::AABB *r_aabb) {
	if (p_shape.is_null() || r_aabb == nullptr) {
		return false;
	}

	const godot::Ref<godot::ArrayMesh> debug_mesh = p_shape->get_debug_mesh();
	if (debug_mesh.is_null()) {
		return false;
	}

	*r_aabb = debug_mesh->get_aabb();
	return true;
}

static bool _try_get_collision_shape_aabb(godot::CollisionShape3D *p_collision_shape, godot::AABB *r_aabb) {
	if (p_collision_shape == nullptr) {
		return false;
	}

	return _try_get_shape_local_aabb(p_collision_shape->get_shape(), r_aabb);
}

static bool _try_get_collision_object_aabb(godot::CollisionObject3D *p_collision_object, godot::AABB *r_aabb) {
	if (p_collision_object == nullptr || r_aabb == nullptr) {
		return false;
	}

	const godot::PackedInt32Array owner_ids = p_collision_object->get_shape_owners();
	int best_shape_index = INT32_MAX;
	bool found = false;
	for (int32_t owner_pos = 0; owner_pos < owner_ids.size(); ++owner_pos) {
		const uint32_t owner_id = (uint32_t)owner_ids[owner_pos];
		if (p_collision_object->is_shape_owner_disabled(owner_id)) {
			continue;
		}

		const int32_t shape_count = p_collision_object->shape_owner_get_shape_count(owner_id);
		for (int32_t shape_pos = 0; shape_pos < shape_count; ++shape_pos) {
			const int32_t shape_index = p_collision_object->shape_owner_get_shape_index(owner_id, shape_pos);
			if (shape_index < 0 || shape_index >= best_shape_index) {
				continue;
			}

			godot::AABB local_aabb;
			if (!_try_get_shape_local_aabb(p_collision_object->shape_owner_get_shape(owner_id, shape_pos), &local_aabb)) {
				continue;
			}

			best_shape_index = shape_index;
			*r_aabb = p_collision_object->shape_owner_get_transform(owner_id).xform(local_aabb);
			found = true;
		}
	}

	return found;
}

// get_aabb(node_id) -> pos_x, pos_y, pos_z, size_x, size_y, size_z
// 获取主碰撞体在节点自身坐标系下的 AABB。
static int l_get_aabb(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::Node3D *node = _resolve_node(node_id, "get_aabb");
	if (node == nullptr) {
		return _push_zero_aabb(p_L);
	}

	godot::AABB aabb;
	if (_try_get_collision_shape_aabb(godot::Object::cast_to<godot::CollisionShape3D>(node), &aabb)) {
		return _push_aabb(p_L, aabb);
	}

	if (_try_get_collision_object_aabb(godot::Object::cast_to<godot::CollisionObject3D>(node), &aabb)) {
		return _push_aabb(p_L, aabb);
	}

	return _push_zero_aabb(p_L);
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

static const luaL_Reg physics_funcs[] = {
	{"get_aabb", l_get_aabb},
	{"move_and_slide", l_move_and_slide},
	{"set_velocity", l_set_velocity},
	{"get_velocity", l_get_velocity},
	{"get_real_velocity", l_get_real_velocity},
	{"is_on_floor", l_is_on_floor},
	{"is_on_wall", l_is_on_wall},
	{"is_on_ceiling", l_is_on_ceiling},
	{"get_floor_normal", l_get_floor_normal},
	{nullptr, nullptr}
};

int luaopen_native_physics(lua_State *p_L) {
	luaL_newlib(p_L, physics_funcs);
	return 1;
}

} // namespace luagd
