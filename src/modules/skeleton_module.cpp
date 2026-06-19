#include "skeleton_module.h"

#include "node_module.h"

#include <godot_cpp/classes/skeleton3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector3.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

static godot::ObjectID _read_node_id(lua_State *p_L, int p_index) {
	return godot::ObjectID((uint64_t)luaL_checkinteger(p_L, p_index));
}

static godot::Skeleton3D *_resolve_skeleton(godot::ObjectID p_node_id, const char *p_func_name) {
	if (p_node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_skeleton.", p_func_name, ": node id is 0");
		return nullptr;
	}

	godot::Node3D *node = node_resolve(p_node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_skeleton.", p_func_name, ": node is no longer valid, id ", p_node_id);
		return nullptr;
	}

	godot::Skeleton3D *skeleton = godot::Object::cast_to<godot::Skeleton3D>(node);
	if (skeleton == nullptr) {
		godot::UtilityFunctions::printerr("native_skeleton.", p_func_name, ": node is not Skeleton3D, id ", p_node_id);
		return nullptr;
	}

	return skeleton;
}

static bool _resolve_bone(godot::Skeleton3D *p_skeleton, const char *p_bone_name, int &r_bone_idx, const char *p_func_name) {
	r_bone_idx = p_skeleton->find_bone(godot::String(p_bone_name));
	if (r_bone_idx == -1) {
		godot::UtilityFunctions::printerr("native_skeleton.", p_func_name, ": bone \"", p_bone_name, "\" not found");
		return false;
	}
	return true;
}

// bone_exists(skeleton_node_id, bone_name) -> bool
static int l_bone_exists(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const char *bone_name = luaL_checkstring(p_L, 2);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "bone_exists");
	if (skeleton == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	int bone_idx = skeleton->find_bone(godot::String(bone_name));
	lua_pushboolean(p_L, bone_idx != -1);
	return 1;
}

// get_bone_count(skeleton_node_id) -> count
static int l_get_bone_count(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "get_bone_count");
	if (skeleton == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	lua_pushinteger(p_L, skeleton->get_bone_count());
	return 1;
}

// get_bone_name(skeleton_node_id, bone_idx) -> name
static int l_get_bone_name(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const int bone_idx = luaL_checkinteger(p_L, 2);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "get_bone_name");
	if (skeleton == nullptr) {
		return 0;
	}

	godot::String name = skeleton->get_bone_name(bone_idx);
	lua_pushstring(p_L, name.utf8().get_data());
	return 1;
}

// --- 位置操作 ---

// set_bone_position(skeleton_node_id, bone_name, x, y, z, is_global) -> void
static int l_set_bone_position(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const char *bone_name = luaL_checkstring(p_L, 2);
	const double x = luaL_checknumber(p_L, 3);
	const double y = luaL_checknumber(p_L, 4);
	const double z = luaL_checknumber(p_L, 5);
	const bool is_global = lua_toboolean(p_L, 6);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "set_bone_position");
	if (skeleton == nullptr) {
		return 0;
	}

	int bone_idx;
	if (!_resolve_bone(skeleton, bone_name, bone_idx, "set_bone_position")) {
		return 0;
	}

	if (is_global) {
		godot::Transform3D bone_global = skeleton->get_bone_global_pose(bone_idx);
		godot::Transform3D skeleton_global = skeleton->get_global_transform();
		godot::Transform3D world = skeleton_global * bone_global;
		world.origin = godot::Vector3((float)x, (float)y, (float)z);
		godot::Transform3D new_bone_global = skeleton_global.affine_inverse() * world;
		skeleton->set_bone_global_pose(bone_idx, new_bone_global);
	} else {
		skeleton->set_bone_pose_position(bone_idx, godot::Vector3((float)x, (float)y, (float)z));
	}

	return 0;
}

// get_bone_position(skeleton_node_id, bone_name, is_global) -> x, y, z
static int l_get_bone_position(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const char *bone_name = luaL_checkstring(p_L, 2);
	const bool is_global = lua_toboolean(p_L, 3);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "get_bone_position");
	if (skeleton == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	int bone_idx;
	if (!_resolve_bone(skeleton, bone_name, bone_idx, "get_bone_position")) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	godot::Vector3 pos;
	if (is_global) {
		godot::Transform3D bone_global = skeleton->get_bone_global_pose(bone_idx);
		godot::Transform3D skeleton_global = skeleton->get_global_transform();
		godot::Transform3D world = skeleton_global * bone_global;
		pos = world.origin;
	} else {
		pos = skeleton->get_bone_pose_position(bone_idx);
	}

	lua_pushnumber(p_L, pos.x);
	lua_pushnumber(p_L, pos.y);
	lua_pushnumber(p_L, pos.z);
	return 3;
}

// --- 旋转操作 ---

// set_bone_rotation(skeleton_node_id, bone_name, x, y, z, is_global) -> void
static int l_set_bone_rotation(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const char *bone_name = luaL_checkstring(p_L, 2);
	const double x = luaL_checknumber(p_L, 3);
	const double y = luaL_checknumber(p_L, 4);
	const double z = luaL_checknumber(p_L, 5);
	const bool is_global = lua_toboolean(p_L, 6);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "set_bone_rotation");
	if (skeleton == nullptr) {
		return 0;
	}

	int bone_idx;
	if (!_resolve_bone(skeleton, bone_name, bone_idx, "set_bone_rotation")) {
		return 0;
	}

	const godot::Vector3 euler_rad(
		godot::Math::deg_to_rad((float)x),
		godot::Math::deg_to_rad((float)y),
		godot::Math::deg_to_rad((float)z)
	);

	if (is_global) {
		godot::Transform3D bone_global = skeleton->get_bone_global_pose(bone_idx);
		godot::Transform3D skeleton_global = skeleton->get_global_transform();
		godot::Transform3D world = skeleton_global * bone_global;

		godot::Vector3 world_scale = world.basis.get_scale();
		godot::Basis new_world_basis = godot::Basis::from_euler(euler_rad);
		new_world_basis.scale(world_scale);

		world.basis = new_world_basis;
		godot::Transform3D new_bone_global = skeleton_global.affine_inverse() * world;
		skeleton->set_bone_global_pose(bone_idx, new_bone_global);
	} else {
		godot::Quaternion quat = godot::Quaternion::from_euler(euler_rad);
		skeleton->set_bone_pose_rotation(bone_idx, quat);
	}

	return 0;
}

// get_bone_rotation(skeleton_node_id, bone_name, is_global) -> x, y, z
static int l_get_bone_rotation(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const char *bone_name = luaL_checkstring(p_L, 2);
	const bool is_global = lua_toboolean(p_L, 3);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "get_bone_rotation");
	if (skeleton == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	int bone_idx;
	if (!_resolve_bone(skeleton, bone_name, bone_idx, "get_bone_rotation")) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	godot::Vector3 euler_rad;
	if (is_global) {
		godot::Transform3D bone_global = skeleton->get_bone_global_pose(bone_idx);
		godot::Transform3D skeleton_global = skeleton->get_global_transform();
		godot::Transform3D world = skeleton_global * bone_global;
		euler_rad = world.basis.get_rotation_quaternion().get_euler();
	} else {
		euler_rad = skeleton->get_bone_pose_rotation(bone_idx).get_euler();
	}

	lua_pushnumber(p_L, godot::Math::rad_to_deg(euler_rad.x));
	lua_pushnumber(p_L, godot::Math::rad_to_deg(euler_rad.y));
	lua_pushnumber(p_L, godot::Math::rad_to_deg(euler_rad.z));
	return 3;
}

// --- 缩放操作 ---

// set_bone_scale(skeleton_node_id, bone_name, x, y, z, is_global) -> void
static int l_set_bone_scale(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const char *bone_name = luaL_checkstring(p_L, 2);
	const double x = luaL_checknumber(p_L, 3);
	const double y = luaL_checknumber(p_L, 4);
	const double z = luaL_checknumber(p_L, 5);
	const bool is_global = lua_toboolean(p_L, 6);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "set_bone_scale");
	if (skeleton == nullptr) {
		return 0;
	}

	int bone_idx;
	if (!_resolve_bone(skeleton, bone_name, bone_idx, "set_bone_scale")) {
		return 0;
	}

	const godot::Vector3 new_scale((float)x, (float)y, (float)z);

	if (is_global) {
		godot::Transform3D bone_global = skeleton->get_bone_global_pose(bone_idx);
		godot::Transform3D skeleton_global = skeleton->get_global_transform();
		godot::Transform3D world = skeleton_global * bone_global;

		godot::Quaternion world_rot = world.basis.get_rotation_quaternion();
		godot::Basis new_world_basis;
		new_world_basis.set_quaternion_scale(world_rot, new_scale);

		world.basis = new_world_basis;
		godot::Transform3D new_bone_global = skeleton_global.affine_inverse() * world;
		skeleton->set_bone_global_pose(bone_idx, new_bone_global);
	} else {
		skeleton->set_bone_pose_scale(bone_idx, new_scale);
	}

	return 0;
}

// get_bone_scale(skeleton_node_id, bone_name, is_global) -> x, y, z
static int l_get_bone_scale(lua_State *p_L) {
	const godot::ObjectID skeleton_id = _read_node_id(p_L, 1);
	const char *bone_name = luaL_checkstring(p_L, 2);
	const bool is_global = lua_toboolean(p_L, 3);

	godot::Skeleton3D *skeleton = _resolve_skeleton(skeleton_id, "get_bone_scale");
	if (skeleton == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	int bone_idx;
	if (!_resolve_bone(skeleton, bone_name, bone_idx, "get_bone_scale")) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	godot::Vector3 scale;
	if (is_global) {
		godot::Transform3D bone_global = skeleton->get_bone_global_pose(bone_idx);
		godot::Transform3D skeleton_global = skeleton->get_global_transform();
		godot::Transform3D world = skeleton_global * bone_global;
		scale = world.basis.get_scale();
	} else {
		scale = skeleton->get_bone_pose_scale(bone_idx);
	}

	lua_pushnumber(p_L, scale.x);
	lua_pushnumber(p_L, scale.y);
	lua_pushnumber(p_L, scale.z);
	return 3;
}

static const luaL_Reg skeleton_funcs[] = {
	{"bone_exists", l_bone_exists},
	{"get_bone_count", l_get_bone_count},
	{"get_bone_name", l_get_bone_name},
	{"set_bone_position", l_set_bone_position},
	{"get_bone_position", l_get_bone_position},
	{"set_bone_rotation", l_set_bone_rotation},
	{"get_bone_rotation", l_get_bone_rotation},
	{"set_bone_scale", l_set_bone_scale},
	{"get_bone_scale", l_get_bone_scale},
	{nullptr, nullptr}
};

int luaopen_native_skeleton(lua_State *p_L) {
	luaL_newlib(p_L, skeleton_funcs);
	return 1;
}

} // namespace luagd
