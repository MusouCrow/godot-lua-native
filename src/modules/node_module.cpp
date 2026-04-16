#include "node_module.h"

#include "../host/host_thread_check.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

enum NodeOwnership {
	NODE_OWNERSHIP_REFERENCE = 0,
	NODE_OWNERSHIP_OWNED = 1,
};

struct NodeRecord {
	godot::ObjectID id;
	NodeOwnership ownership;
};

static godot::HashMap<godot::ObjectID, NodeRecord> nodes;
static godot::HashMap<godot::ObjectID, godot::HashSet<godot::ObjectID>> root_children;
static godot::ObjectID root_node_id;

static godot::ObjectID _read_object_id(lua_State *p_L, int p_index) {
	return godot::ObjectID((uint64_t)luaL_checkinteger(p_L, p_index));
}

static godot::Node3D *_resolve_node3d(godot::ObjectID p_id) {
	if (p_id.is_null()) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::Node3D>(godot::ObjectDB::get_instance((uint64_t)p_id));
}

static godot::ObjectID _get_tracking_root_id(godot::Node *p_node) {
	if (p_node == nullptr) {
		return godot::ObjectID();
	}

	godot::Node *owner = p_node->get_owner();
	if (owner == nullptr) {
		return godot::ObjectID(p_node->get_instance_id());
	}

	return godot::ObjectID(owner->get_instance_id());
}

static void _remove_child_from_root_table(godot::ObjectID p_root_id, godot::ObjectID p_child_id) {
	if (!root_children.has(p_root_id)) {
		return;
	}

	root_children[p_root_id].erase(p_child_id);
	if (root_children[p_root_id].is_empty()) {
		root_children.erase(p_root_id);
	}
}

static void _unregister_reference_node(godot::ObjectID p_id) {
	if (!nodes.has(p_id)) {
		return;
	}

	const NodeRecord rec = nodes[p_id];
	if (rec.ownership == NODE_OWNERSHIP_REFERENCE) {
		_remove_child_from_root_table(_get_tracking_root_id(_resolve_node3d(p_id)), p_id);
	}

	nodes.erase(p_id);
}

static godot::ObjectID _register_node(godot::Node3D *p_node, NodeOwnership p_ownership) {
	if (p_node == nullptr) {
		return godot::ObjectID();
	}

	const godot::ObjectID id = godot::ObjectID(p_node->get_instance_id());
	NodeRecord rec;
	rec.id = id;
	rec.ownership = p_ownership;
	nodes[id] = rec;
	return id;
}

static godot::Node3D *_get_record_node(const NodeRecord *p_rec) {
	if (p_rec == nullptr) {
		return nullptr;
	}

	return _resolve_node3d(p_rec->id);
}

static NodeRecord *get_node(godot::ObjectID p_id, const char *p_func_name) {
	if (!nodes.has(p_id)) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": invalid id ", p_id);
		return nullptr;
	}

	NodeRecord *rec = &nodes[p_id];
	if (_resolve_node3d(p_id) == nullptr) {
		_unregister_reference_node(p_id);
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": node is no longer valid, id ", p_id);
		return nullptr;
	}

	return rec;
}

// get_child_by_path(id, path) -> id
// 基于指定节点查找子节点并返回句柄。
static int l_get_child_by_path(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	const char *path = luaL_checkstring(p_L, 2);

	NodeRecord *owner_rec = get_node(id, "get_child_by_path");
	if (owner_rec == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node3D *owner_node = _get_record_node(owner_rec);
	if (owner_node == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	const godot::NodePath node_path((godot::String(path)));
	if (!owner_node->has_node(node_path)) {
		godot::UtilityFunctions::printerr("native_node.get_child_by_path: node not found: ", path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node *found_node = owner_node->get_node<godot::Node>(node_path);
	godot::Node3D *node3d = godot::Object::cast_to<godot::Node3D>(found_node);
	if (node3d == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_child_by_path: node is not a Node3D: ", path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	const godot::ObjectID child_id = _register_node(node3d, NODE_OWNERSHIP_REFERENCE);
	const godot::ObjectID root_id = _get_tracking_root_id(owner_node);
	root_children[root_id].insert(child_id);

	lua_pushinteger(p_L, (int64_t)child_id);
	return 1;
}

// set_root(path) -> bool
// 设置后续实例化的挂载根节点。
static int l_set_root(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(godot::Engine::get_singleton()->get_main_loop());
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_node.set_root: SceneTree not available");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Window *root_window = tree->get_root();
	if (root_window == nullptr) {
		godot::UtilityFunctions::printerr("native_node.set_root: Scene root not available");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *window_node = godot::Object::cast_to<godot::Node>(root_window);
	const godot::NodePath node_path((godot::String(path)));
	godot::Node *found_node = window_node->get_node<godot::Node>(node_path);
	if (found_node == nullptr) {
		godot::UtilityFunctions::printerr("native_node.set_root: node not found: ", path);
		lua_pushboolean(p_L, false);
		return 1;
	}

	root_node_id = godot::ObjectID(found_node->get_instance_id());
	lua_pushboolean(p_L, true);
	return 1;
}

// instantiate(scene_path) -> id
// 加载并实例化场景，挂到当前根节点下。
static int l_instantiate(lua_State *p_L) {
	const char *scene_path = luaL_checkstring(p_L, 1);

	if (root_node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_node.instantiate: root not set, call set_root first");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node *root_node = godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance((uint64_t)root_node_id));
	if (root_node == nullptr || !root_node->is_inside_tree()) {
		godot::UtilityFunctions::printerr("native_node.instantiate: root node is no longer valid");
		root_node_id = godot::ObjectID();
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Ref<godot::Resource> resource = godot::ResourceLoader::get_singleton()->load(godot::String(scene_path));
	if (resource.is_null()) {
		godot::UtilityFunctions::printerr("native_node.instantiate: failed to load resource: ", scene_path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::PackedScene *packed_scene = godot::Object::cast_to<godot::PackedScene>(resource.ptr());
	if (packed_scene == nullptr) {
		godot::UtilityFunctions::printerr("native_node.instantiate: resource is not a PackedScene: ", scene_path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node *instance = packed_scene->instantiate();
	if (instance == nullptr) {
		godot::UtilityFunctions::printerr("native_node.instantiate: failed to instantiate scene: ", scene_path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node3D *node3d = godot::Object::cast_to<godot::Node3D>(instance);
	if (node3d == nullptr) {
		godot::UtilityFunctions::printerr("native_node.instantiate: instantiated node is not a Node3D: ", scene_path);
		instance->queue_free();
		lua_pushinteger(p_L, -1);
		return 1;
	}

	root_node->add_child(instance);
	const godot::ObjectID id = _register_node(node3d, NODE_OWNERSHIP_OWNED);
	lua_pushinteger(p_L, (int64_t)id);
	return 1;
}

// destroy(id) -> void
// 销毁创建节点或释放引用节点。
static int l_destroy(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	if (!nodes.has(id)) {
		return 0;
	}

	const NodeRecord rec = nodes[id];
	if (root_children.has(id)) {
		godot::HashSet<godot::ObjectID> child_ids = root_children[id];
		for (godot::HashSet<godot::ObjectID>::Iterator it = child_ids.begin(); it != child_ids.end(); ++it) {
			_unregister_reference_node(*it);
		}
		root_children.erase(id);
	}

	if (rec.ownership == NODE_OWNERSHIP_OWNED) {
		godot::Node3D *node = _resolve_node3d(id);
		if (node != nullptr && node->is_inside_tree()) {
			node->queue_free();
		}
	} else {
		_remove_child_from_root_table(_get_tracking_root_id(_resolve_node3d(id)), id);
	}

	nodes.erase(id);
	return 0;
}

// is_valid(id) -> bool
// 检查节点引用是否仍然有效。
static int l_is_valid(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	if (!nodes.has(id)) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node3D *node = _resolve_node3d(id);
	lua_pushboolean(p_L, node != nullptr && node->is_inside_tree());
	return 1;
}

// get_name(id) -> string
// 获取节点名称。
static int l_get_name(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	NodeRecord *rec = get_node(id, "get_name");
	if (rec == nullptr) {
		return 0;
	}

	const godot::CharString utf8_name = godot::String(_get_record_node(rec)->get_name()).utf8();
	lua_pushstring(p_L, utf8_name.get_data());
	return 1;
}

// get_type(id) -> string
// 返回节点当前的 Godot 运行时类名。
static int l_get_type(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	NodeRecord *rec = get_node(id, "get_type");
	if (rec == nullptr) {
		lua_pushstring(p_L, "");
		return 1;
	}

	const godot::CharString utf8_type = godot::String(_get_record_node(rec)->get_class()).utf8();
	lua_pushstring(p_L, utf8_type.get_data());
	return 1;
}

static const luaL_Reg node_funcs[] = {
	{"set_root", l_set_root},
	{"instantiate", l_instantiate},
	{"destroy", l_destroy},
	{"get_child_by_path", l_get_child_by_path},
	{"is_valid", l_is_valid},
	{"get_name", l_get_name},
	{"get_type", l_get_type},
	{nullptr, nullptr}
};

int luaopen_native_node(lua_State *p_L) {
	luaL_newlib(p_L, node_funcs);
	return 1;
}

void node_cleanup() {
	nodes.clear();
	root_children.clear();
	root_node_id = godot::ObjectID();
}

godot::Node3D *node_resolve(godot::ObjectID p_id) {
	if (!ensure_main_thread("native_node.node_resolve")) {
		return nullptr;
	}

	if (!nodes.has(p_id)) {
		return nullptr;
	}

	godot::Node3D *node = _resolve_node3d(p_id);
	if (node == nullptr || !node->is_inside_tree()) {
		return nullptr;
	}

	return node;
}

} // namespace luagd
