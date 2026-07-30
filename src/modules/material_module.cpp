#include "material_module.h"

#include "node_module.h"

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
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

// set_param_color(node_id, param_name, r, g, b, a) -> bool
// 在节点的直接子节点中设置实例着色器颜色参数。
static int l_set_param_color(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const char *param_name = luaL_checkstring(p_L, 2);
	const double r = luaL_checknumber(p_L, 3);
	const double g = luaL_checknumber(p_L, 4);
	const double b = luaL_checknumber(p_L, 5);
	const double a = luaL_checknumber(p_L, 6);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_param_color: node id is 0");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_param_color: node is no longer valid, id ", node_id);
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const godot::Color color((float)r, (float)g, (float)b, (float)a);
	const godot::StringName param_name_sn(param_name);
	bool applied = false;

	for (int64_t i = 0; i < root_node->get_child_count(); ++i) {
		godot::Node *child = root_node->get_child(i);
		if (child == nullptr) {
			continue;
		}

		godot::GeometryInstance3D *geometry = godot::Object::cast_to<godot::GeometryInstance3D>(child);
		if (geometry == nullptr) {
			continue;
		}

		geometry->set_instance_shader_parameter(param_name_sn, color);
		applied = true;
	}

	lua_pushboolean(p_L, applied);
	return 1;
}

// 遍历节点自身及其直接子节点，对GeometryInstance3D执行操作
template<typename Func>
static int _apply_to_self_and_children(godot::Node *p_root, Func p_func) {
	int count = 0;

	godot::GeometryInstance3D *self_geometry = godot::Object::cast_to<godot::GeometryInstance3D>(p_root);
	if (self_geometry != nullptr) {
		p_func(self_geometry);
		count++;
	}

	for (int64_t i = 0; i < p_root->get_child_count(); ++i) {
		godot::Node *child = p_root->get_child(i);
		if (child == nullptr) {
			continue;
		}

		godot::GeometryInstance3D *child_geometry = godot::Object::cast_to<godot::GeometryInstance3D>(child);
		if (child_geometry != nullptr) {
			p_func(child_geometry);
			count++;
		}
	}

	return count;
}

// set_material_override(node_id, material_path) -> count
// 设置节点自身及其直接子节点的material_override属性。
static int l_set_material_override(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const char *material_path = luaL_checkstring(p_L, 2);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_material_override: node id is 0");
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_material_override: node is no longer valid, id ", node_id);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Ref<godot::Resource> resource = godot::ResourceLoader::get_singleton()->load(godot::String(material_path));
	if (resource.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_material_override: failed to load material: ", material_path);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Material *material = godot::Object::cast_to<godot::Material>(resource.ptr());
	if (material == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_material_override: resource is not a Material: ", material_path);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	int count = _apply_to_self_and_children(root_node, [material](godot::GeometryInstance3D *geom) {
		geom->set_material_override(material);
	});

	lua_pushinteger(p_L, count);
	return 1;
}

// set_transparency(node_id, transparency) -> count
// 设置节点自身及其直接子节点的transparency属性。
static int l_set_transparency(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double transparency = luaL_checknumber(p_L, 2);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_transparency: node id is 0");
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_transparency: node is no longer valid, id ", node_id);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	int count = _apply_to_self_and_children(root_node, [transparency](godot::GeometryInstance3D *geom) {
		geom->set_transparency((float)transparency);
	});

	lua_pushinteger(p_L, count);
	return 1;
}

static const luaL_Reg material_funcs[] = {
	{"set_param_color", l_set_param_color},
	{"set_material_override", l_set_material_override},
	{"set_transparency", l_set_transparency},
	{nullptr, nullptr}
};

int luaopen_native_material(lua_State *p_L) {
	luaL_newlib(p_L, material_funcs);
	return 1;
}

} // namespace luagd
