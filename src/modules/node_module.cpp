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

static godot::Node *_resolve_node(godot::ObjectID p_id) {
	if (p_id.is_null()) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance((uint64_t)p_id));
}

static godot::Node *_get_scene_root_node(const char *p_func_name) {
	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(godot::Engine::get_singleton()->get_main_loop());
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": SceneTree not available");
		return nullptr;
	}

	godot::Window *root_window = tree->get_root();
	if (root_window == nullptr) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": Scene root not available");
		return nullptr;
	}

	return godot::Object::cast_to<godot::Node>(root_window);
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
		_remove_child_from_root_table(_get_tracking_root_id(_resolve_node(p_id)), p_id);
	}

	nodes.erase(p_id);
}

static godot::ObjectID _register_node(godot::Node *p_node, NodeOwnership p_ownership) {
	if (p_node == nullptr) {
		return godot::ObjectID();
	}

	const godot::ObjectID id = godot::ObjectID(p_node->get_instance_id());
	if (nodes.has(id)) {
		return id;
	}

	NodeRecord rec;
	rec.id = id;
	rec.ownership = p_ownership;
	nodes[id] = rec;
	return id;
}

static godot::Node *_get_record_node(const NodeRecord *p_rec) {
	if (p_rec == nullptr) {
		return nullptr;
	}

	return _resolve_node(p_rec->id);
}

static NodeRecord *get_node(godot::ObjectID p_id, const char *p_func_name) {
	if (!nodes.has(p_id)) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": invalid id ", p_id);
		return nullptr;
	}

	NodeRecord *rec = &nodes[p_id];
	if (_resolve_node(p_id) == nullptr) {
		_unregister_reference_node(p_id);
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": node is no longer valid, id ", p_id);
		return nullptr;
	}

	return rec;
}

// get_node_by_path(path) -> id
// 基于全局节点路径查找节点并返回句柄。
static int l_get_node_by_path(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);

	godot::Node *root_node = _get_scene_root_node("get_node_by_path");
	if (root_node == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	const godot::NodePath node_path((godot::String(path)));
	if (!root_node->has_node(node_path)) {
		godot::UtilityFunctions::printerr("native_node.get_node_by_path: node not found: ", path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node *found_node = root_node->get_node<godot::Node>(node_path);
	const godot::ObjectID node_id = _register_node(found_node, NODE_OWNERSHIP_REFERENCE);
	lua_pushinteger(p_L, (int64_t)node_id);
	return 1;
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

	godot::Node *owner_node = _get_record_node(owner_rec);
	if (owner_node == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	const godot::NodePath node_path((godot::String(path)));
	if (!owner_node->has_node(node_path)) {
		// 查找失败属于正常情况，不输出错误，由调用方判断返回值
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node *found_node = owner_node->get_node<godot::Node>(node_path);
	const godot::ObjectID child_id = _register_node(found_node, NODE_OWNERSHIP_REFERENCE);
	const godot::ObjectID root_id = _get_tracking_root_id(owner_node);
	root_children[root_id].insert(child_id);

	lua_pushinteger(p_L, (int64_t)child_id);
	return 1;
}

// has_child_by_path(id, path) -> bool
// 纯查询：判断已登记节点下是否存在指定路径的子节点。
// 与 get_child_by_path 的差异：不注册节点、不修改引用记录，只返回存在性布尔值，
// 因此适合作为探测接口反复调用。
// 返回：存在 true；句柄无效、节点已失效或路径不存在时返回 false。
static int l_has_child_by_path(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	const char *path = luaL_checkstring(p_L, 2);

	// 纯查询语义：无效句柄视为“不存在”，不报错
	if (!nodes.has(id)) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *owner_node = _resolve_node(id);
	if (owner_node == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const godot::NodePath node_path((godot::String(path)));
	lua_pushboolean(p_L, owner_node->has_node(node_path));
	return 1;
}

// find_child_by_name(id, pattern, recursive?, owned?) -> id
// 在指定节点的后代中按名称模式查找第一个匹配节点并注册为引用句柄，
// 等价 Godot Node::find_child。
// 匹配：String::match，大小写敏感，'*' 匹配任意长度，'?' 匹配单字符；
// 按场景树顺序返回首个匹配。
// 范围：后代节点，不含自身；recursive=false 时仅直接子节点。
// 注意：owned=true 仅匹配 owner 有效的后代；本项目运行时挂载或 set_parent 移动的
// 节点 owner 多为空，故缺省 false 以覆盖全部后代。
// 返回：匹配子节点句柄；未找到、pattern 为空或句柄无效时返回 -1（查找失败不输出错误）。
static int l_find_child_by_name(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	const char *pattern = luaL_checkstring(p_L, 2);

	// 缺省 recursive=true：与 Godot 默认一致，也符合本接口的用途
	bool recursive = true;
	if (!lua_isnoneornil(p_L, 3)) {
		recursive = lua_toboolean(p_L, 3) != 0;
	}
	const bool owned = lua_toboolean(p_L, 4);

	NodeRecord *owner_rec = get_node(id, "find_child_by_name");
	if (owner_rec == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node *owner_node = _get_record_node(owner_rec);
	if (owner_node == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	const godot::String name_pattern(pattern);
	if (name_pattern.is_empty()) {
		godot::UtilityFunctions::printerr("native_node.find_child_by_name: empty pattern");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node *found_node = owner_node->find_child(name_pattern, recursive, owned);
	if (found_node == nullptr) {
		// 查找失败属于正常情况，不输出错误，由调用方判断返回值
		lua_pushinteger(p_L, -1);
		return 1;
	}

	const godot::ObjectID child_id = _register_node(found_node, NODE_OWNERSHIP_REFERENCE);
	const godot::ObjectID root_id = _get_tracking_root_id(owner_node);
	root_children[root_id].insert(child_id);

	lua_pushinteger(p_L, (int64_t)child_id);
	return 1;
}

// set_root(path) -> bool
// 设置后续实例化的挂载根节点。
static int l_set_root(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);

	godot::Node *window_node = _get_scene_root_node("set_root");
	if (window_node == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const godot::NodePath node_path((godot::String(path)));
	if (!window_node->has_node(node_path)) {
		godot::UtilityFunctions::printerr("native_node.set_root: node not found: ", path);
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *found_node = window_node->get_node<godot::Node>(node_path);
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

	root_node->add_child(instance);
	const godot::ObjectID id = _register_node(instance, NODE_OWNERSHIP_OWNED);
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
		godot::Node *node = _resolve_node(id);
		if (node != nullptr && node->is_inside_tree()) {
			node->queue_free();
		}
	} else {
		_remove_child_from_root_table(_get_tracking_root_id(_resolve_node(id)), id);
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

	godot::Node *node = _resolve_node(id);
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

// set_name(id, name) -> string|false
// 设置节点名称。
// 注意：Godot 会对同级重名自动改名，故返回最终生效的名称供调用方校验。
// 返回：最终生效的名称；句柄无效时返回 false。
static int l_set_name(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	const char *name = luaL_checkstring(p_L, 2);

	if (!ensure_main_thread("native_node.set_name")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	NodeRecord *rec = get_node(id, "set_name");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *node = _get_record_node(rec);
	if (node == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	node->set_name(godot::String(name));
	const godot::CharString utf8_name = godot::String(node->get_name()).utf8();
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

// get_child_count(id, include_internal) -> count
// 获取节点直接子节点数量。
// 返回：子节点数量，节点无效时返回 -1。
static int l_get_child_count(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	const bool include_internal = lua_toboolean(p_L, 2);

	NodeRecord *rec = get_node(id, "get_child_count");
	if (rec == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	const int32_t count = _get_record_node(rec)->get_child_count(include_internal);
	lua_pushinteger(p_L, count);
	return 1;
}

// find_registered_ancestor(node_id) -> ancestor_id
// 从给定的 ObjectID 向上查找第一个已注册的祖先节点。
static int l_find_registered_ancestor(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);

	// 先检查节点本身是否存在
	godot::Node *node = godot::Object::cast_to<godot::Node>(godot::ObjectDB::get_instance((uint64_t)id));
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_node.find_registered_ancestor: node not found, id ", id);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 向上遍历查找已注册的节点
	godot::Node *current = node;
	while (current != nullptr) {
		godot::ObjectID current_id = godot::ObjectID(current->get_instance_id());

		// 检查当前节点是否已注册
		if (nodes.has(current_id)) {
			lua_pushinteger(p_L, (int64_t)current_id);
			return 1;
		}

		// 向上移动到父节点
		current = current->get_parent();
	}

	// 没有找到已注册的祖先
	lua_pushinteger(p_L, -1);
	return 1;
}

// set_parent(node_id, parent_node_id, keep_global_transform?) -> bool
// 将已登记节点移动到新的已登记父节点下，等价 Godot Node::reparent。
// 约束：目标父节点须在场景树内，且不是自身或自身后代（否则 Godot 会先把节点
// 从旧父节点摘下，再因循环检测失败而不回挂）。
// 注意：本函数不改变节点 owner，也不调整 root_children 归属；归属语义仍由
// get_child_by_path 的查询起点决定。
// 返回：成功 true；句柄无效、节点无父节点或会造成循环时返回 false。
static int l_set_parent(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	const godot::ObjectID parent_id = _read_object_id(p_L, 2);
	const bool keep_global_transform = lua_toboolean(p_L, 3);

	NodeRecord *rec = get_node(id, "set_parent");
	NodeRecord *parent_rec = get_node(parent_id, "set_parent");
	if (rec == nullptr || parent_rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node *node = _get_record_node(rec);
	godot::Node *new_parent = _get_record_node(parent_rec);
	if (node == nullptr || new_parent == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	// 与 Godot reparent 前置条件一致：节点必须已有父节点
	if (node->get_parent() == nullptr) {
		godot::UtilityFunctions::printerr("native_node.set_parent: node has no parent");
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (!new_parent->is_inside_tree()) {
		godot::UtilityFunctions::printerr("native_node.set_parent: new parent is not inside tree");
		lua_pushboolean(p_L, false);
		return 1;
	}

	// 防循环：必须在 reparent 前拦截，否则节点会被摘下且无法回挂
	if (node == new_parent || node->is_ancestor_of(new_parent)) {
		godot::UtilityFunctions::printerr("native_node.set_parent: cannot parent to self or descendant");
		lua_pushboolean(p_L, false);
		return 1;
	}

	// 同父时 Godot 内部直接返回，保持 no-op 成功语义
	node->reparent(new_parent, keep_global_transform);
	lua_pushboolean(p_L, true);
	return 1;
}

static const luaL_Reg node_funcs[] = {
	{"set_root", l_set_root},
	{"instantiate", l_instantiate},
	{"destroy", l_destroy},
	{"get_node_by_path", l_get_node_by_path},
	{"get_child_by_path", l_get_child_by_path},
	{"has_child_by_path", l_has_child_by_path},
	{"find_child_by_name", l_find_child_by_name},
	{"is_valid", l_is_valid},
	{"get_name", l_get_name},
	{"set_name", l_set_name},
	{"get_type", l_get_type},
	{"get_child_count", l_get_child_count},
	{"find_registered_ancestor", l_find_registered_ancestor},
	{"set_parent", l_set_parent},
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

godot::Node *node_resolve_any(godot::ObjectID p_id) {
	if (!ensure_main_thread("native_node.node_resolve_any")) {
		return nullptr;
	}

	if (!nodes.has(p_id)) {
		return nullptr;
	}

	godot::Node *node = _resolve_node(p_id);
	if (node == nullptr || !node->is_inside_tree()) {
		return nullptr;
	}

	return node;
}

godot::Node3D *node_resolve(godot::ObjectID p_id) {
	godot::Node *node = node_resolve_any(p_id);
	if (node == nullptr) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::Node3D>(node);
}

} // namespace luagd
