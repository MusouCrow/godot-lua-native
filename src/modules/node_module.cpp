#include "node_module.h"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 节点类型枚举
enum NodeType {
	NODE_TYPE_NODE3D = 0,
	NODE_TYPE_CHARACTER_BODY3D = 1,
	NODE_TYPE_CAMERA3D = 2
};

// 节点所有权类型
enum NodeOwnership {
	NODE_OWNERSHIP_REFERENCE = 0,  // 引用，destroy 时不销毁
	NODE_OWNERSHIP_OWNED = 1       // 创建，destroy 时销毁
};

// 节点记录
struct NodeRecord {
	godot::ObjectID id;
	NodeType type;        // 节点类型
	NodeOwnership ownership;  // 所有权类型
};

// 模块级静态数据
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

static NodeType _detect_node_type(godot::Node *p_node) {
	if (godot::Object::cast_to<godot::Camera3D>(p_node) != nullptr) {
		return NODE_TYPE_CAMERA3D;
	}

	if (godot::Object::cast_to<godot::CharacterBody3D>(p_node) != nullptr) {
		return NODE_TYPE_CHARACTER_BODY3D;
	}

	return NODE_TYPE_NODE3D;
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

	NodeRecord rec = nodes[p_id];
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
	rec.type = _detect_node_type(p_node);
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

// 获取节点记录，不存在时打印错误
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

// ============================================================================
// 节点引用管理
// ============================================================================

// get_child_by_path(id, path) -> id
// 基于指定节点查找子节点并返回句柄。
// 返回子节点 id，失败返回 -1。
static int l_get_child_by_path(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);
	const char *path = luaL_checkstring(p_L, 2);

	NodeRecord *owner_rec = get_node(id, "get_child_by_path");
	if (owner_rec == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Node3D *owner_node = _resolve_node3d(owner_rec->id);
	if (owner_node == nullptr) {
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::NodePath node_path((godot::String(path)));
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
// 设置根节点路径，后续创建的节点将挂载到此节点下。
// 路径格式如 "/root/pre_entry/pre_scene/root"。
// 返回是否设置成功。
static int l_set_root(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(
		godot::Engine::get_singleton()->get_main_loop()
	);
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
	godot::NodePath node_path((godot::String(path)));
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
// 从场景资源路径加载并实例化节点，挂载到根节点下。
// scene_path: 资源路径，如 "res://assets/char/sm_char_proto.glb"
// 返回节点 id，失败返回 -1。
// 注意：必须先调用 set_root 设置根节点。
static int l_instantiate(lua_State *p_L) {
	const char *scene_path = luaL_checkstring(p_L, 1);

	// 检查根节点是否已设置
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

	// 加载场景资源
	godot::Ref<godot::Resource> resource = godot::ResourceLoader::get_singleton()->load(
		godot::String(scene_path)
	);
	if (resource.is_null()) {
		godot::UtilityFunctions::printerr("native_node.instantiate: failed to load resource: ", scene_path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 转换为 PackedScene
	godot::PackedScene *packed_scene = godot::Object::cast_to<godot::PackedScene>(resource.ptr());
	if (packed_scene == nullptr) {
		godot::UtilityFunctions::printerr("native_node.instantiate: resource is not a PackedScene: ", scene_path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 实例化场景
	godot::Node *instance = packed_scene->instantiate();
	if (instance == nullptr) {
		godot::UtilityFunctions::printerr("native_node.instantiate: failed to instantiate scene: ", scene_path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 检查是否为 Node3D
	godot::Node3D *node3d = godot::Object::cast_to<godot::Node3D>(instance);
	if (node3d == nullptr) {
		godot::UtilityFunctions::printerr("native_node.instantiate: instantiated node is not a Node3D: ", scene_path);
		instance->queue_free();  // 清理
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 添加到根节点
	root_node->add_child(instance);

	const godot::ObjectID id = _register_node(node3d, NODE_OWNERSHIP_OWNED);
	lua_pushinteger(p_L, (int64_t)id);
	return 1;
}

// destroy(id) -> void
// 销毁节点并释放引用。
// 对于通过 instantiate 创建的节点，调用 queue_free 销毁节点。
// 对于通过 get_child_by_path 获取的节点，仅释放引用。
static int l_destroy(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);

	if (!nodes.has(id)) {
		// 重复释放不报错，静默处理
		return 0;
	}

	NodeRecord rec = nodes[id];
	if (root_children.has(id)) {
		godot::HashSet<godot::ObjectID> child_ids = root_children[id];
		for (godot::HashSet<godot::ObjectID>::Iterator it = child_ids.begin(); it != child_ids.end(); ++it) {
			_unregister_reference_node(*it);
		}
		root_children.erase(id);
	}

	// 如果是创建的节点且节点仍有效，销毁它
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
// 检查节点引用是否有效。
static int l_is_valid(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);

	if (!nodes.has(id)) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Node3D *node = _resolve_node3d(id);
	bool valid = node != nullptr && node->is_inside_tree();
	lua_pushboolean(p_L, valid);
	return 1;
}

// ============================================================================
// 位置
// ============================================================================

// set_position(id, x, y, z, is_global) -> void
// 设置节点位置。
// is_global: true 为世界坐标，false 为局部坐标（默认）。
static int l_set_position(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool is_global = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "set_position");
	if (rec == nullptr) {
		return 0;
	}
	godot::Node3D *node = _get_record_node(rec);

	godot::Vector3 pos((float)x, (float)y, (float)z);
	if (is_global) {
		node->set_global_position(pos);
	} else {
		node->set_position(pos);
	}
	return 0;
}

// get_position(id, is_global) -> x, y, z
// 获取节点位置。
// is_global: true 为世界坐标，false 为局部坐标（默认）。
static int l_get_position(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);

	NodeRecord *rec = get_node(id, "get_position");
	if (rec == nullptr) {
		return 0;
	}
	godot::Node3D *node = _get_record_node(rec);

	godot::Vector3 pos = is_global ? node->get_global_position()
	                                : node->get_position();
	lua_pushnumber(p_L, pos.x);
	lua_pushnumber(p_L, pos.y);
	lua_pushnumber(p_L, pos.z);
	return 3;
}

// ============================================================================
// 旋转
// ============================================================================

// set_rotation(id, x, y, z, is_global) -> void
// 设置节点旋转（度数）。
// is_global: true 为世界旋转，false 为局部旋转（默认）。
static int l_set_rotation(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool is_global = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "set_rotation");
	if (rec == nullptr) {
		return 0;
	}
	godot::Node3D *node = _get_record_node(rec);

	godot::Vector3 rot((float)x, (float)y, (float)z);
	if (is_global) {
		node->set_global_rotation_degrees(rot);
	} else {
		node->set_rotation_degrees(rot);
	}
	return 0;
}

// get_rotation(id, is_global) -> x, y, z
// 获取节点旋转（度数）。
// is_global: true 为世界旋转，false 为局部旋转（默认）。
static int l_get_rotation(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);

	NodeRecord *rec = get_node(id, "get_rotation");
	if (rec == nullptr) {
		return 0;
	}
	godot::Node3D *node = _get_record_node(rec);

	godot::Vector3 rot = is_global ? node->get_global_rotation_degrees()
	                                : node->get_rotation_degrees();
	lua_pushnumber(p_L, rot.x);
	lua_pushnumber(p_L, rot.y);
	lua_pushnumber(p_L, rot.z);
	return 3;
}

// look_at(id, target_x, target_y, target_z, use_model_front) -> void
// 使节点朝向目标位置。
// use_model_front: true 时 +Z 轴（模型前向）指向目标，false 时 -Z 轴指向目标（默认 false）。
static int l_look_at(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool use_model_front = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "look_at");
	if (rec == nullptr) {
		return 0;
	}
	godot::Node3D *node = _get_record_node(rec);

	godot::Vector3 target((float)x, (float)y, (float)z);
	godot::Vector3 up(0.0f, 1.0f, 0.0f);
	node->look_at(target, up, use_model_front);
	return 0;
}

// get_forward(id, is_global, use_model_front) -> x, y, z
// 获取节点的前向向量（归一化）。
// is_global: true 为世界空间前向，false 为局部前向（默认）。
// use_model_front: true 返回 +Z 轴（模型前向），false 返回 -Z 轴（相机前向，默认）。
static int l_get_forward(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);
	bool use_model_front = lua_toboolean(p_L, 3);

	NodeRecord *rec = get_node(id, "get_forward");
	if (rec == nullptr) {
		return 0;
	}
	godot::Node3D *node = _get_record_node(rec);

	godot::Basis basis = is_global ? node->get_global_transform().basis
	                                : node->get_transform().basis;
	godot::Vector3 forward = use_model_front ? basis.get_column(2) : -basis.get_column(2);
	lua_pushnumber(p_L, forward.x);
	lua_pushnumber(p_L, forward.y);
	lua_pushnumber(p_L, forward.z);
	return 3;
}

// ============================================================================
// 移动
// ============================================================================

// move_and_slide(id) -> bool
// 执行移动并滑动。
// 应在 physics_process 中调用。
// 仅支持 CharacterBody3D 节点。
// 返回是否发生碰撞。
static int l_move_and_slide(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "move_and_slide");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.move_and_slide: node is not CharacterBody3D");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	bool collided = body->move_and_slide();
	lua_pushboolean(p_L, collided);
	return 1;
}

// set_velocity(id, x, y, z) -> void
// 设置节点的速度向量。
// 仅支持 CharacterBody3D 节点。
// 注意：不要乘以 delta，move_and_slide 会自动处理。
static int l_set_velocity(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);

	NodeRecord *rec = get_node(id, "set_velocity");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.set_velocity: node is not CharacterBody3D");
		return 0;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	godot::Vector3 vel((float)x, (float)y, (float)z);
	body->set_velocity(vel);
	return 0;
}

// get_velocity(id) -> x, y, z
// 获取节点的速度向量。
// 仅支持 CharacterBody3D 节点。
static int l_get_velocity(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "get_velocity");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.get_velocity: node is not CharacterBody3D");
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	godot::Vector3 vel = body->get_velocity();
	lua_pushnumber(p_L, vel.x);
	lua_pushnumber(p_L, vel.y);
	lua_pushnumber(p_L, vel.z);
	return 3;
}

// get_real_velocity(id) -> x, y, z
// 获取节点的实际移动速度。
// 仅支持 CharacterBody3D 节点。
// 考虑滑动后的实际速度。
static int l_get_real_velocity(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "get_real_velocity");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.get_real_velocity: node is not CharacterBody3D");
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	godot::Vector3 vel = body->get_real_velocity();
	lua_pushnumber(p_L, vel.x);
	lua_pushnumber(p_L, vel.y);
	lua_pushnumber(p_L, vel.z);
	return 3;
}

// is_on_floor(id) -> bool
// 检查节点是否在地面上。
// 仅支持 CharacterBody3D 节点。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_floor(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "is_on_floor");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.is_on_floor: node is not CharacterBody3D");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	lua_pushboolean(p_L, body->is_on_floor());
	return 1;
}

// is_on_wall(id) -> bool
// 检查节点是否在墙上。
// 仅支持 CharacterBody3D 节点。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_wall(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "is_on_wall");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.is_on_wall: node is not CharacterBody3D");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	lua_pushboolean(p_L, body->is_on_wall());
	return 1;
}

// is_on_ceiling(id) -> bool
// 检查节点是否在天花板上。
// 仅支持 CharacterBody3D 节点。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_ceiling(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "is_on_ceiling");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.is_on_ceiling: node is not CharacterBody3D");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	lua_pushboolean(p_L, body->is_on_ceiling());
	return 1;
}

// get_floor_normal(id) -> x, y, z
// 获取地面的碰撞法线。
// 仅支持 CharacterBody3D 节点。
// 仅在 move_and_slide 调用后且 is_on_floor 为 true 时有效。
static int l_get_floor_normal(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "get_floor_normal");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->type != NODE_TYPE_CHARACTER_BODY3D) {
		godot::UtilityFunctions::printerr("native_node.get_floor_normal: node is not CharacterBody3D");
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(_get_record_node(rec));
	godot::Vector3 normal = body->get_floor_normal();
	lua_pushnumber(p_L, normal.x);
	lua_pushnumber(p_L, normal.y);
	lua_pushnumber(p_L, normal.z);
	return 3;
}

// ============================================================================
// 信息
// ============================================================================

// get_name(id) -> string
// 获取节点名称。
static int l_get_name(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "get_name");
	if (rec == nullptr) {
		return 0;
	}

	godot::String name = _get_record_node(rec)->get_name();
	godot::CharString utf8_name = name.utf8();
	lua_pushstring(p_L, utf8_name.get_data());
	return 1;
}

// get_type(id) -> string
// 获取节点类型。
// 返回 "Node3D" 或 "CharacterBody3D"。
static int l_get_type(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "get_type");
	if (rec == nullptr) {
		lua_pushstring(p_L, "");
		return 1;
	}

	if (rec->type == NODE_TYPE_CHARACTER_BODY3D) {
		lua_pushstring(p_L, "CharacterBody3D");
	} else if (rec->type == NODE_TYPE_CAMERA3D) {
		lua_pushstring(p_L, "Camera3D");
	} else {
		lua_pushstring(p_L, "Node3D");
	}
	return 1;
}

// ============================================================================
// 相机
// ============================================================================

// set_fov(id, fov) -> void
// 设置相机的视场角（FOV）。
// 仅对 Camera3D 节点有效。
// fov: 视场角（度）
static int l_set_fov(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);
	double fov = luaL_checknumber(p_L, 2);

	NodeRecord *rec = get_node(id, "set_fov");
	if (rec == nullptr) {
		return 0;
	}

	godot::Camera3D *camera = godot::Object::cast_to<godot::Camera3D>(_get_record_node(rec));
	if (camera == nullptr) {
		godot::UtilityFunctions::printerr("native_node.set_fov: node is not Camera3D");
		return 0;
	}

	camera->set_fov((float)fov);
	return 0;
}

// get_fov(id) -> fov
// 获取相机的视场角（FOV）。
// 仅对 Camera3D 节点有效。
// 返回: 视场角（度）
static int l_get_fov(lua_State *p_L) {
	godot::ObjectID id = _read_object_id(p_L, 1);

	NodeRecord *rec = get_node(id, "get_fov");
	if (rec == nullptr) {
		return 0;
	}

	godot::Camera3D *camera = godot::Object::cast_to<godot::Camera3D>(_get_record_node(rec));
	if (camera == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_fov: node is not Camera3D");
		lua_pushnumber(p_L, 0);
		return 1;
	}

	float fov = camera->get_fov();
	lua_pushnumber(p_L, fov);
	return 1;
}

// ============================================================================
// 模块注册
// ============================================================================

static const luaL_Reg node_funcs[] = {
	// 节点引用管理
	{"set_root", l_set_root},
	{"instantiate", l_instantiate},
	{"destroy", l_destroy},
	{"get_child_by_path", l_get_child_by_path},
	{"is_valid", l_is_valid},

	// 位置
	{"set_position", l_set_position},
	{"get_position", l_get_position},

	// 旋转
	{"set_rotation", l_set_rotation},
	{"get_rotation", l_get_rotation},
	{"look_at", l_look_at},
	{"get_forward", l_get_forward},

	// 移动
	{"move_and_slide", l_move_and_slide},
	{"set_velocity", l_set_velocity},
	{"get_velocity", l_get_velocity},
	{"get_real_velocity", l_get_real_velocity},
	{"is_on_floor", l_is_on_floor},
	{"is_on_wall", l_is_on_wall},
	{"is_on_ceiling", l_is_on_ceiling},
	{"get_floor_normal", l_get_floor_normal},

	// 信息
	{"get_name", l_get_name},
	{"get_type", l_get_type},

	// 相机
	{"set_fov", l_set_fov},
	{"get_fov", l_get_fov},

	{nullptr, nullptr}
};

int luaopen_native_node(lua_State *p_L) {
	luaL_newlib(p_L, node_funcs);
	return 1;
}

void node_cleanup() {
	// GDExtension 反初始化阶段只清理模块记录，场景对象交给引擎统一销毁。
	nodes.clear();
	root_children.clear();
	root_node_id = godot::ObjectID();
}

godot::Node3D *node_resolve(godot::ObjectID p_id) {
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
