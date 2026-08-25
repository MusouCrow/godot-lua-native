#include "camera_module.h"

#include "node_module.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/math.hpp>
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

// unproject_position(node_id, x, y, z) -> screen_x, screen_y
// 将世界坐标投影为视口内 2D 屏幕坐标。
static int l_unproject_position(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);

	godot::Camera3D *camera = _resolve_camera(node_id, "unproject_position");
	if (camera == nullptr) {
		return 0;
	}

	const godot::Vector3 world_point((float)x, (float)y, (float)z);
	const godot::Vector2 screen_point = camera->unproject_position(world_point);
	lua_pushnumber(p_L, screen_point.x);
	lua_pushnumber(p_L, screen_point.y);
	return 2;
}

// get_ground_footprint_aabb(node_id, ground_y) -> min_x, max_x, min_z, max_z
// 将当前 viewport 四角射线投射到 y = ground_y 平面，返回 X/Z 轴对齐包围范围。
// 覆盖 FOV、相机位置、俯角、滚转、宽高比、缩放距离。
// 约束：任一角射线平行地面(dir.y≈0)或交点在相机背后(t<0)时视为无效，返回 4 个 0。
static int l_get_ground_footprint_aabb(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double ground_y = luaL_checknumber(p_L, 2);

	godot::Camera3D *camera = _resolve_camera(node_id, "get_ground_footprint_aabb");
	if (camera == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 4;
	}

	godot::Viewport *viewport = camera->get_viewport();
	if (viewport == nullptr) {
		godot::UtilityFunctions::printerr("native_camera.get_ground_footprint_aabb: no viewport, id ", node_id);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 4;
	}

	const godot::Vector2 size = viewport->get_visible_rect().size;
	const godot::Vector2 corners[4] = {
		godot::Vector2(0.0f, 0.0f),
		godot::Vector2(size.x, 0.0f),
		godot::Vector2(0.0f, size.y),
		godot::Vector2(size.x, size.y)
	};

	double min_x = 0.0, max_x = 0.0, min_z = 0.0, max_z = 0.0;
	bool has_point = false;

	// 约束：dir.y 近 0 视为平行地面；t<0 表示交点在相机背后。任一无效则整体失败返回 4 个 0。
	for (int i = 0; i < 4; i++) {
		const godot::Vector3 origin = camera->project_ray_origin(corners[i]);
		const godot::Vector3 dir = godot::Vector3(camera->project_ray_normal(corners[i]));

		if (godot::Math::abs(dir.y) < 1e-6f) {
			godot::UtilityFunctions::printerr("native_camera.get_ground_footprint_aabb: ray parallel to ground, id ", node_id);
			lua_pushnumber(p_L, 0);
			lua_pushnumber(p_L, 0);
			lua_pushnumber(p_L, 0);
			lua_pushnumber(p_L, 0);
			return 4;
		}

		const double t = ((double)ground_y - (double)origin.y) / (double)dir.y;
		if (t < 0.0) {
			godot::UtilityFunctions::printerr("native_camera.get_ground_footprint_aabb: intersection behind camera, id ", node_id);
			lua_pushnumber(p_L, 0);
			lua_pushnumber(p_L, 0);
			lua_pushnumber(p_L, 0);
			lua_pushnumber(p_L, 0);
			return 4;
		}

		const double px = (double)origin.x + (double)dir.x * t;
		const double pz = (double)origin.z + (double)dir.z * t;

		if (!has_point) {
			min_x = max_x = px;
			min_z = max_z = pz;
			has_point = true;
		} else {
			if (px < min_x) min_x = px;
			if (px > max_x) max_x = px;
			if (pz < min_z) min_z = pz;
			if (pz > max_z) max_z = pz;
		}
	}

	lua_pushnumber(p_L, min_x);
	lua_pushnumber(p_L, max_x);
	lua_pushnumber(p_L, min_z);
	lua_pushnumber(p_L, max_z);
	return 4;
}

static const luaL_Reg camera_funcs[] = {
	{"set_fov", l_set_fov},
	{"get_fov", l_get_fov},
	{"unproject_position", l_unproject_position},
	{"get_ground_footprint_aabb", l_get_ground_footprint_aabb},
	{nullptr, nullptr}
};

int luaopen_native_camera(lua_State *p_L) {
	luaL_newlib(p_L, camera_funcs);
	return 1;
}

} // namespace luagd
