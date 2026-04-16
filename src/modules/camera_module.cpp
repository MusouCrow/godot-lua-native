#include "camera_module.h"

#include "node_module.h"

#include <godot_cpp/classes/camera3d.hpp>
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

static godot::Camera3D *_resolve_camera(godot::ObjectID p_node_id, const char *p_func_name) {
	if (p_node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_camera.", p_func_name, ": node id is 0");
		return nullptr;
	}

	godot::Node3D *node = node_resolve(p_node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_camera.", p_func_name, ": node is no longer valid, id ", p_node_id);
		return nullptr;
	}

	godot::Camera3D *camera = godot::Object::cast_to<godot::Camera3D>(node);
	if (camera == nullptr) {
		godot::UtilityFunctions::printerr("native_camera.", p_func_name, ": node is not Camera3D, id ", p_node_id);
		return nullptr;
	}

	return camera;
}

// set_fov(node_id, fov) -> void
// 设置相机视场角。
static int l_set_fov(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double fov = luaL_checknumber(p_L, 2);

	godot::Camera3D *camera = _resolve_camera(node_id, "set_fov");
	if (camera == nullptr) {
		return 0;
	}

	camera->set_fov((float)fov);
	return 0;
}

// get_fov(node_id) -> fov
// 获取相机视场角。
static int l_get_fov(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::Camera3D *camera = _resolve_camera(node_id, "get_fov");
	if (camera == nullptr) {
		lua_pushnumber(p_L, 0);
		return 1;
	}

	lua_pushnumber(p_L, camera->get_fov());
	return 1;
}

static const luaL_Reg camera_funcs[] = {
	{"set_fov", l_set_fov},
	{"get_fov", l_get_fov},
	{nullptr, nullptr}
};

int luaopen_native_camera(lua_State *p_L) {
	luaL_newlib(p_L, camera_funcs);
	return 1;
}

} // namespace luagd
