#include "material_module.h"

#include "node_module.h"

#include <godot_cpp/classes/geometry_instance3d.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/color.hpp>
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

// 复制 MeshInstance3D 的 material_override 及所有 surface_override_material。
// 浅复制即可，避免材质复用导致动画驱动产生问题。
static void _duplicate_mesh_materials(godot::MeshInstance3D *p_mesh) {
	if (p_mesh == nullptr) {
		return;
	}

	godot::Ref<godot::Material> material_override = p_mesh->get_material_override();
	if (!material_override.is_null()) {
		godot::Ref<godot::Material> duplicated = material_override->duplicate(false);
		if (!duplicated.is_null()) {
			p_mesh->set_material_override(duplicated);
		}
	}

	const int32_t surface_count = p_mesh->get_surface_override_material_count();
	for (int32_t i = 0; i < surface_count; ++i) {
		godot::Ref<godot::Material> surface_material = p_mesh->get_surface_override_material(i);
		if (surface_material.is_null()) {
			continue;
		}

		godot::Ref<godot::Material> duplicated = surface_material->duplicate(false);
		if (!duplicated.is_null()) {
			p_mesh->set_surface_override_material(i, duplicated);
		}
	}
}

// 对节点的直接子节点设置实例着色器参数的通用模板函数
template<typename ParamType>
static bool _set_shader_parameter_to_children(
		godot::Node *p_root,
		const godot::StringName &p_param_name,
		const ParamType &p_value) {
	bool applied = false;

	for (int64_t i = 0; i < p_root->get_child_count(); ++i) {
		godot::Node *child = p_root->get_child(i);
		if (child == nullptr) {
			continue;
		}

		godot::GeometryInstance3D *geometry = godot::Object::cast_to<godot::GeometryInstance3D>(child);
		if (geometry == nullptr) {
			continue;
		}

		geometry->set_instance_shader_parameter(p_param_name, p_value);
		applied = true;
	}

	return applied;
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

	bool applied = _set_shader_parameter_to_children(root_node, param_name_sn, color);

	lua_pushboolean(p_L, applied);
	return 1;
}

// set_param_vec3(node_id, param_name, x, y, z) -> bool
// 在节点的直接子节点中设置实例着色器Vector3参数。
static int l_set_param_vec3(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const char *param_name = luaL_checkstring(p_L, 2);
	const double x = luaL_checknumber(p_L, 3);
	const double y = luaL_checknumber(p_L, 4);
	const double z = luaL_checknumber(p_L, 5);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_param_vec3: node id is 0");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_param_vec3: node is no longer valid, id ", node_id);
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const godot::Vector3 vec3((float)x, (float)y, (float)z);
	const godot::StringName param_name_sn(param_name);

	bool applied = _set_shader_parameter_to_children(root_node, param_name_sn, vec3);

	lua_pushboolean(p_L, applied);
	return 1;
}

// set_param_float(node_id, param_name, value) -> bool
// 在节点的直接子节点中设置实例着色器float参数。
static int l_set_param_float(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const char *param_name = luaL_checkstring(p_L, 2);
	const double value = luaL_checknumber(p_L, 3);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_param_float: node id is 0");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_param_float: node is no longer valid, id ", node_id);
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const float float_value = (float)value;
	const godot::StringName param_name_sn(param_name);

	bool applied = _set_shader_parameter_to_children(root_node, param_name_sn, float_value);

	lua_pushboolean(p_L, applied);
	return 1;
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

	godot::Ref<godot::Material> material = resource;
	if (material.is_null()) {
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

// set_material_overlay(node_id, material_path) -> count
// 设置节点自身及其直接子节点的material_overlay属性。
// material_path为nil时清空overlay。
static int l_set_material_overlay(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.set_material_overlay: node id is 0");
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.set_material_overlay: node is no longer valid, id ", node_id);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Ref<godot::Material> material;

	// material_path为nil时清空overlay，否则加载材质资源
	if (!lua_isnoneornil(p_L, 2)) {
		const char *material_path = luaL_checkstring(p_L, 2);

		godot::Ref<godot::Resource> resource = godot::ResourceLoader::get_singleton()->load(godot::String(material_path));
		if (resource.is_null()) {
			godot::UtilityFunctions::printerr("native_material.set_material_overlay: failed to load material: ", material_path);
			lua_pushinteger(p_L, 0);
			return 1;
		}

		material = resource;
		if (material.is_null()) {
			godot::UtilityFunctions::printerr("native_material.set_material_overlay: resource is not a Material: ", material_path);
			lua_pushinteger(p_L, 0);
			return 1;
		}
	}

	int count = _apply_to_self_and_children(root_node, [material](godot::GeometryInstance3D *geom) {
		geom->set_material_overlay(material);
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

// enable_cast_shadow(node_id, enabled) -> count
// 设置节点自身及其直接子节点的阴影投射开关。
// enabled: true=投射阴影(ON)，false=关闭阴影(OFF)。
static int l_enable_cast_shadow(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const bool enabled = lua_toboolean(p_L, 2);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.enable_cast_shadow: node id is 0");
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.enable_cast_shadow: node is no longer valid, id ", node_id);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	const godot::GeometryInstance3D::ShadowCastingSetting shadow_mode = enabled
			? godot::GeometryInstance3D::SHADOW_CASTING_SETTING_ON
			: godot::GeometryInstance3D::SHADOW_CASTING_SETTING_OFF;

	int count = _apply_to_self_and_children(root_node, [shadow_mode](godot::GeometryInstance3D *geom) {
		geom->set_cast_shadows_setting(shadow_mode);
	});

	lua_pushinteger(p_L, count);
	return 1;
}

// duplicate_materials(node_id) -> count
// 复制节点自身及其直接子节点（MeshInstance3D）的材质。
// 复制 material_override 与所有 surface_override_material，避免材质复用导致动画驱动产生问题。
static int l_duplicate_materials(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);

	if (node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_material.duplicate_materials: node id is 0");
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node3D *root_node_3d = node_resolve(node_id);
	if (root_node_3d == nullptr) {
		godot::UtilityFunctions::printerr("native_material.duplicate_materials: node is no longer valid, id ", node_id);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(root_node_3d);
	if (root_node == nullptr) {
		lua_pushinteger(p_L, 0);
		return 1;
	}

	int count = 0;

	godot::MeshInstance3D *self_mesh = godot::Object::cast_to<godot::MeshInstance3D>(root_node);
	if (self_mesh != nullptr) {
		_duplicate_mesh_materials(self_mesh);
		count++;
	}

	for (int64_t i = 0; i < root_node->get_child_count(); ++i) {
		godot::Node *child = root_node->get_child(i);
		if (child == nullptr) {
			continue;
		}

		godot::MeshInstance3D *child_mesh = godot::Object::cast_to<godot::MeshInstance3D>(child);
		if (child_mesh != nullptr) {
			_duplicate_mesh_materials(child_mesh);
			count++;
		}
	}

	lua_pushinteger(p_L, count);
	return 1;
}

static const luaL_Reg material_funcs[] = {
	{"set_param_color", l_set_param_color},
	{"set_param_vec3", l_set_param_vec3},
	{"set_param_float", l_set_param_float},
	{"set_material_override", l_set_material_override},
	{"set_material_overlay", l_set_material_overlay},
	{"set_transparency", l_set_transparency},
	{"enable_cast_shadow", l_enable_cast_shadow},
	{"duplicate_materials", l_duplicate_materials},
	{nullptr, nullptr}
};

int luaopen_native_material(lua_State *p_L) {
	luaL_newlib(p_L, material_funcs);
	return 1;
}

} // namespace luagd
