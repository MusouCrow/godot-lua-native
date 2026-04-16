#include "material_module.h"

#include "node_module.h"

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

static godot::ObjectID _read_node_id(lua_State *p_L, int p_index) {
	return godot::ObjectID((uint64_t)luaL_checkinteger(p_L, p_index));
}

// set_param_color_in_group(node_id, group_name, param_name, r, g, b, a) -> bool
// 在当前节点子树内按组设置实例着色器颜色参数。
static int l_set_param_color_in_group(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const char *group_name = luaL_checkstring(p_L, 2);
	const char *param_name = luaL_checkstring(p_L, 3);
	const double r = luaL_checknumber(p_L, 4);
	const double g = luaL_checknumber(p_L, 5);
	const double b = luaL_checknumber(p_L, 6);
	const double a = luaL_checknumber(p_L, 7);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_param_color_in_group: node id is 0");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_param_color_in_group: node is no longer valid, id ", node_id);
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr || !root_node->is_inside_tree()) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::SceneTree *tree = root_node->get_tree();
	if (tree == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const godot::Array candidates = tree->get_nodes_in_group(godot::StringName(group_name));
	const godot::Color color((float)r, (float)g, (float)b, (float)a);
	bool applied = false;
	for (int64_t i = 0; i < candidates.size(); ++i) {
		godot::Node *candidate = godot::Object::cast_to<godot::Node>(candidates[i]);
		if (candidate == nullptr) {
			continue;
		}

		if (candidate != root_node && !root_node->is_ancestor_of(candidate)) {
			continue;
		}

		godot::GeometryInstance3D *geometry = godot::Object::cast_to<godot::GeometryInstance3D>(candidate);
		if (geometry == nullptr) {
			continue;
		}

		geometry->set_instance_shader_parameter(godot::StringName(param_name), color);
		applied = true;
	}

	lua_pushboolean(p_L, applied);
	return 1;
}

static const luaL_Reg material_funcs[] = {
	{"set_param_color_in_group", l_set_param_color_in_group},
	{nullptr, nullptr}
};

int luaopen_native_material(lua_State *p_L) {
	luaL_newlib(p_L, material_funcs);
	return 1;
}

} // namespace luagd
