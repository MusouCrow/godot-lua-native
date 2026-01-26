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
	int32_t id;
	godot::Node3D *node;  // 改为 Node3D*，支持所有 3D 节点
	NodeType type;        // 节点类型
	NodeOwnership ownership;  // 所有权类型
};

// 模块级静态数据
static godot::HashMap<int32_t, NodeRecord> nodes;
static int32_t next_id = 1;
static godot::Node *root_node = nullptr;  // 根节点指针

// 获取节点记录，不存在时打印错误
static NodeRecord *get_node(int32_t p_id, const char *p_func_name) {
	if (!nodes.has(p_id)) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": invalid id ", p_id);
		return nullptr;
	}
	NodeRecord *rec = &nodes[p_id];

	// 检查节点是否仍有效
	if (rec->node == nullptr || !rec->node->is_inside_tree()) {
		godot::UtilityFunctions::printerr("native_node.", p_func_name, ": node is no longer valid, id ", p_id);
		return nullptr;
	}

	return rec;
}

// ============================================================================
// 节点引用管理
// ============================================================================

// get_by_path(path) -> id
// 通过场景路径获取节点句柄。
// 支持 Node3D 及其所有派生类（包括 CharacterBody3D）。
// 返回节点 id，失败返回 -1。
static int l_get_by_path(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(
		godot::Engine::get_singleton()->get_main_loop()
	);
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: SceneTree not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::Window *root_window = tree->get_root();
	if (root_window == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: Scene root not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// Window 继承自 Viewport，需要找到实际的根节点
	// get_root() 返回的 Window 就是场景树的根，可以作为节点访问
	godot::Node *root = godot::Object::cast_to<godot::Node>(root_window);
	if (root == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: Scene root is not a Node");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::NodePath node_path((godot::String(path)));
	godot::Node *found_node = root->get_node<godot::Node>(node_path);
	if (found_node == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: node not found: ", path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 检查是否为 Node3D 或其派生类
	godot::Node3D *node3d = godot::Object::cast_to<godot::Node3D>(found_node);
	if (node3d == nullptr) {
		godot::UtilityFunctions::printerr("native_node.get_by_path: node is not a Node3D: ", path);
		lua_pushinteger(p_L, -1);
		return 1;
	}

	NodeRecord rec;
	rec.id = next_id++;
	rec.node = node3d;
	rec.ownership = NODE_OWNERSHIP_REFERENCE;  // 标记为引用

	// 检测节点类型（按优先级：Camera3D > CharacterBody3D > Node3D）
	godot::Camera3D *camera = godot::Object::cast_to<godot::Camera3D>(found_node);
	if (camera != nullptr) {
		rec.type = NODE_TYPE_CAMERA3D;
	} else {
		godot::CharacterBody3D *body = godot::Object::cast_to<godot::CharacterBody3D>(found_node);
		rec.type = (body != nullptr) ? NODE_TYPE_CHARACTER_BODY3D : NODE_TYPE_NODE3D;
	}

	nodes[rec.id] = rec;

	lua_pushinteger(p_L, rec.id);
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

	root_node = found_node;
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
	if (root_node == nullptr) {
		godot::UtilityFunctions::printerr("native_node.instantiate: root not set, call set_root first");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 检查根节点是否仍有效
	if (!root_node->is_inside_tree()) {
		godot::UtilityFunctions::printerr("native_node.instantiate: root node is no longer valid");
		root_node = nullptr;
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

	// 创建记录
	NodeRecord rec;
	rec.id = next_id++;
	rec.node = node3d;
	rec.ownership = NODE_OWNERSHIP_OWNED;  // 标记为创建的节点

	// 检测节点类型（按优先级：Camera3D > CharacterBody3D > Node3D）
	godot::Camera3D *camera = godot::Object::cast_to<godot::Camera3D>(instance);
	if (camera != nullptr) {
		rec.type = NODE_TYPE_CAMERA3D;
	} else {
		godot::CharacterBody3D *body = godot::Object::cast_to<godot::CharacterBody3D>(instance);
		rec.type = (body != nullptr) ? NODE_TYPE_CHARACTER_BODY3D : NODE_TYPE_NODE3D;
	}

	nodes[rec.id] = rec;

	lua_pushinteger(p_L, rec.id);
	return 1;
}

// destroy(id) -> void
// 销毁节点并释放引用。
// 对于通过 instantiate 创建的节点，调用 queue_free 销毁节点。
// 对于通过 get_by_path 获取的节点，仅释放引用。
static int l_destroy(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	if (!nodes.has(id)) {
		// 重复释放不报错，静默处理
		return 0;
	}

	NodeRecord *rec = &nodes[id];

	// 如果是创建的节点且节点仍有效，销毁它
	if (rec->ownership == NODE_OWNERSHIP_OWNED &&
		rec->node != nullptr &&
		rec->node->is_inside_tree()) {
		rec->node->queue_free();
	}

	nodes.erase(id);
	return 0;
}

// is_valid(id) -> bool
// 检查节点引用是否有效。
static int l_is_valid(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	if (!nodes.has(id)) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	NodeRecord *rec = &nodes[id];
	bool valid = rec->node != nullptr && rec->node->is_inside_tree();
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
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool is_global = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "set_position");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 pos((float)x, (float)y, (float)z);
	if (is_global) {
		rec->node->set_global_position(pos);
	} else {
		rec->node->set_position(pos);
	}
	return 0;
}

// get_position(id, is_global) -> x, y, z
// 获取节点位置。
// is_global: true 为世界坐标，false 为局部坐标（默认）。
static int l_get_position(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);

	NodeRecord *rec = get_node(id, "get_position");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 pos = is_global ? rec->node->get_global_position()
	                                : rec->node->get_position();
	lua_pushnumber(p_L, pos.x);
	lua_pushnumber(p_L, pos.y);
	lua_pushnumber(p_L, pos.z);
	return 3;
}

// ============================================================================
// 旋转
// ============================================================================

// set_rotation(id, x, y, z, is_global) -> void
// 设置节点旋转（弧度）。
// is_global: true 为世界旋转，false 为局部旋转（默认）。
static int l_set_rotation(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool is_global = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "set_rotation");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 rot((float)x, (float)y, (float)z);
	if (is_global) {
		rec->node->set_global_rotation(rot);
	} else {
		rec->node->set_rotation(rot);
	}
	return 0;
}

// get_rotation(id, is_global) -> x, y, z
// 获取节点旋转（弧度）。
// is_global: true 为世界旋转，false 为局部旋转（默认）。
static int l_get_rotation(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);

	NodeRecord *rec = get_node(id, "get_rotation");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 rot = is_global ? rec->node->get_global_rotation()
	                                : rec->node->get_rotation();
	lua_pushnumber(p_L, rot.x);
	lua_pushnumber(p_L, rot.y);
	lua_pushnumber(p_L, rot.z);
	return 3;
}

// look_at(id, target_x, target_y, target_z, use_model_front) -> void
// 使节点朝向目标位置。
// use_model_front: true 时 +Z 轴（模型前向）指向目标，false 时 -Z 轴指向目标（默认 false）。
static int l_look_at(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);
	bool use_model_front = lua_toboolean(p_L, 5);

	NodeRecord *rec = get_node(id, "look_at");
	if (rec == nullptr) {
		return 0;
	}

	godot::Vector3 target((float)x, (float)y, (float)z);
	godot::Vector3 up(0.0f, 1.0f, 0.0f);
	rec->node->look_at(target, up, use_model_front);
	return 0;
}

// get_forward(id, is_global, use_model_front) -> x, y, z
// 获取节点的前向向量（归一化）。
// is_global: true 为世界空间前向，false 为局部前向（默认）。
// use_model_front: true 返回 +Z 轴（模型前向），false 返回 -Z 轴（相机前向，默认）。
static int l_get_forward(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	bool is_global = lua_toboolean(p_L, 2);
	bool use_model_front = lua_toboolean(p_L, 3);

	NodeRecord *rec = get_node(id, "get_forward");
	if (rec == nullptr) {
		return 0;
	}

	godot::Basis basis = is_global ? rec->node->get_global_transform().basis
	                                : rec->node->get_transform().basis;
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
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
	bool collided = body->move_and_slide();
	lua_pushboolean(p_L, collided);
	return 1;
}

// set_velocity(id, x, y, z) -> void
// 设置节点的速度向量。
// 仅支持 CharacterBody3D 节点。
// 注意：不要乘以 delta，move_and_slide 会自动处理。
static int l_set_velocity(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
	godot::Vector3 vel((float)x, (float)y, (float)z);
	body->set_velocity(vel);
	return 0;
}

// get_velocity(id) -> x, y, z
// 获取节点的速度向量。
// 仅支持 CharacterBody3D 节点。
static int l_get_velocity(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
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
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
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
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
	lua_pushboolean(p_L, body->is_on_floor());
	return 1;
}

// is_on_wall(id) -> bool
// 检查节点是否在墙上。
// 仅支持 CharacterBody3D 节点。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_wall(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
	lua_pushboolean(p_L, body->is_on_wall());
	return 1;
}

// is_on_ceiling(id) -> bool
// 检查节点是否在天花板上。
// 仅支持 CharacterBody3D 节点。
// 仅在 move_and_slide 调用后有效。
static int l_is_on_ceiling(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
	lua_pushboolean(p_L, body->is_on_ceiling());
	return 1;
}

// get_floor_normal(id) -> x, y, z
// 获取地面的碰撞法线。
// 仅支持 CharacterBody3D 节点。
// 仅在 move_and_slide 调用后且 is_on_floor 为 true 时有效。
static int l_get_floor_normal(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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

	godot::CharacterBody3D *body = static_cast<godot::CharacterBody3D*>(rec->node);
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
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "get_name");
	if (rec == nullptr) {
		return 0;
	}

	godot::String name = rec->node->get_name();
	godot::CharString utf8_name = name.utf8();
	lua_pushstring(p_L, utf8_name.get_data());
	return 1;
}

// get_type(id) -> string
// 获取节点类型。
// 返回 "Node3D" 或 "CharacterBody3D"。
static int l_get_type(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

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
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double fov = luaL_checknumber(p_L, 2);

	NodeRecord *rec = get_node(id, "set_fov");
	if (rec == nullptr) {
		return 0;
	}

	godot::Camera3D *camera = godot::Object::cast_to<godot::Camera3D>(rec->node);
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
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	NodeRecord *rec = get_node(id, "get_fov");
	if (rec == nullptr) {
		return 0;
	}

	godot::Camera3D *camera = godot::Object::cast_to<godot::Camera3D>(rec->node);
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
	{"get_by_path", l_get_by_path},  // 已废弃，保留兼容
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
	// 销毁所有创建的节点
	for (const godot::KeyValue<int32_t, NodeRecord> &kv : nodes) {
		if (kv.value.ownership == NODE_OWNERSHIP_OWNED &&
			kv.value.node != nullptr &&
			kv.value.node->is_inside_tree()) {
			kv.value.node->queue_free();
		}
	}

	// 清除引用和重置状态
	nodes.clear();
	next_id = 1;
	root_node = nullptr;  // 清除根节点引用
}

} // namespace luagd
