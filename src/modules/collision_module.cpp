#include "collision_module.h"

#include "node_module.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/cylinder_shape3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_shape_query_parameters3d.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/rb_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 预设Shape资源，用于intersect_hitbox复用
static godot::Ref<godot::CylinderShape3D> cached_cylinder_shape;
static godot::Ref<godot::BoxShape3D> cached_box_shape;
static godot::Ref<godot::PhysicsShapeQueryParameters3D> cached_query_params;

static godot::ObjectID _read_node_id(lua_State *p_L, int p_index) {
	return godot::ObjectID((uint64_t)luaL_checkinteger(p_L, p_index));
}

static godot::Node3D *_resolve_node(godot::ObjectID p_node_id, const char *p_func_name) {
	if (p_node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_collision.", p_func_name, ": node id is 0");
		return nullptr;
	}

	godot::Node3D *node = node_resolve(p_node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_collision.", p_func_name, ": node is no longer valid, id ", p_node_id);
		return nullptr;
	}

	return node;
}

static int _push_aabb(lua_State *p_L, const godot::AABB &p_aabb) {
	lua_pushnumber(p_L, p_aabb.position.x);
	lua_pushnumber(p_L, p_aabb.position.y);
	lua_pushnumber(p_L, p_aabb.position.z);
	lua_pushnumber(p_L, p_aabb.size.x);
	lua_pushnumber(p_L, p_aabb.size.y);
	lua_pushnumber(p_L, p_aabb.size.z);
	return 6;
}

static int _push_zero_aabb(lua_State *p_L) {
	return _push_aabb(p_L, godot::AABB());
}

static bool _try_get_shape_local_aabb(const godot::Ref<godot::Shape3D> &p_shape, godot::AABB *r_aabb) {
	if (p_shape.is_null() || r_aabb == nullptr) {
		return false;
	}

	const godot::Ref<godot::ArrayMesh> debug_mesh = p_shape->get_debug_mesh();
	if (debug_mesh.is_null()) {
		return false;
	}

	*r_aabb = debug_mesh->get_aabb();
	return true;
}

static bool _try_get_collision_shape_aabb(godot::CollisionShape3D *p_collision_shape, godot::AABB *r_aabb) {
	if (p_collision_shape == nullptr) {
		return false;
	}

	return _try_get_shape_local_aabb(p_collision_shape->get_shape(), r_aabb);
}

static bool _try_get_collision_object_aabb(godot::CollisionObject3D *p_collision_object, godot::AABB *r_aabb) {
	if (p_collision_object == nullptr || r_aabb == nullptr) {
		return false;
	}

	const godot::PackedInt32Array owner_ids = p_collision_object->get_shape_owners();
	int best_shape_index = INT32_MAX;
	bool found = false;
	for (int32_t owner_pos = 0; owner_pos < owner_ids.size(); ++owner_pos) {
		const uint32_t owner_id = (uint32_t)owner_ids[owner_pos];
		if (p_collision_object->is_shape_owner_disabled(owner_id)) {
			continue;
		}

		const int32_t shape_count = p_collision_object->shape_owner_get_shape_count(owner_id);
		for (int32_t shape_pos = 0; shape_pos < shape_count; ++shape_pos) {
			const int32_t shape_index = p_collision_object->shape_owner_get_shape_index(owner_id, shape_pos);
			if (shape_index < 0 || shape_index >= best_shape_index) {
				continue;
			}

			godot::AABB local_aabb;
			if (!_try_get_shape_local_aabb(p_collision_object->shape_owner_get_shape(owner_id, shape_pos), &local_aabb)) {
				continue;
			}

			best_shape_index = shape_index;
			*r_aabb = p_collision_object->shape_owner_get_transform(owner_id).xform(local_aabb);
			found = true;
		}
	}

	return found;
}

// get_aabb(node_id) -> pos_x, pos_y, pos_z, size_x, size_y, size_z
// 获取主碰撞体在节点自身坐标系下的 AABB。
static int l_get_aabb(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::Node3D *node = _resolve_node(node_id, "get_aabb");
	if (node == nullptr) {
		return _push_zero_aabb(p_L);
	}

	godot::AABB aabb;
	if (_try_get_collision_shape_aabb(godot::Object::cast_to<godot::CollisionShape3D>(node), &aabb)) {
		return _push_aabb(p_L, aabb);
	}

	if (_try_get_collision_object_aabb(godot::Object::cast_to<godot::CollisionObject3D>(node), &aabb)) {
		return _push_aabb(p_L, aabb);
	}

	return _push_zero_aabb(p_L);
}

// 判断节点是否为AttackHitbox3D
// AttackHitbox3D是GDScript定义的类，get_class()返回基类Node3D
// 通过检查是否有shape_type属性来判断
static bool _is_attack_hitbox(godot::Node3D *p_node) {
	if (!p_node) {
		return false;
	}
	// 检查是否有shape_type属性（AttackHitbox3D的特征属性）
	godot::Variant shape_type_var = p_node->get("shape_type");
	return shape_type_var.get_type() != godot::Variant::NIL;
}

// 收集直接子节点中的所有AttackHitbox3D（仅一层）
static void _collect_attack_hitboxes(godot::Node *p_parent_node, godot::Vector<godot::Node3D *> &r_hitboxes) {
	if (!p_parent_node) {
		return;
	}

	for (int i = 0; i < p_parent_node->get_child_count(); i++) {
		godot::Node *child = p_parent_node->get_child(i);
		godot::Node3D *child3d = godot::Object::cast_to<godot::Node3D>(child);

		if (child3d && _is_attack_hitbox(child3d)) {
			r_hitboxes.push_back(child3d);
		}
	}
}

// 核心查询逻辑：执行形状碰撞检测，支持扇形过滤和可选去重
static bool _perform_shape_query(
		lua_State *p_L,
		godot::Node3D *p_reference_node,
		const godot::Ref<godot::Shape3D> &p_shape,
		const godot::Transform3D &p_transform,
		uint32_t p_collision_mask,
		bool p_use_sector_filter,
		double p_sector_angle,         // 度
		int p_callback_index,
		godot::RBSet<uint64_t> *p_processed_ids) {  // nullptr = 不去重

	cached_query_params->set_shape(p_shape);
	cached_query_params->set_transform(p_transform);
	cached_query_params->set_collision_mask(p_collision_mask);

	godot::Ref<godot::World3D> world = p_reference_node->get_world_3d();
	if (world.is_null()) {
		godot::UtilityFunctions::printerr("native_collision: reference node not in world");
		return false;
	}

	godot::PhysicsDirectSpaceState3D *space_state = world->get_direct_space_state();
	if (!space_state) {
		godot::UtilityFunctions::printerr("native_collision: failed to get space state");
		return false;
	}

	godot::TypedArray<godot::Dictionary> results = space_state->intersect_shape(cached_query_params);

	godot::Vector3 query_pos;
	godot::Vector3 query_forward;
	double half_angle_rad = 0.0;

	if (p_use_sector_filter) {
		query_pos = p_transform.get_origin();
		query_forward = p_transform.basis.get_column(2); // +Z轴
		half_angle_rad = (p_sector_angle / 2.0) * Math_PI / 180.0;
	}

	for (int i = 0; i < results.size(); i++) {
		godot::Dictionary result = results[i];
		uint64_t target_id = (uint64_t)result["collider_id"];

		// 扇形角度过滤
		if (p_use_sector_filter) {
			godot::Object *collider = (godot::Object *)result["collider"];
			godot::Node3D *target_node = godot::Object::cast_to<godot::Node3D>(collider);

			if (target_node) {
				godot::Vector3 to_target = target_node->get_global_position() - query_pos;
				to_target.y = 0; // 投影到XZ平面

				if (to_target.length_squared() > 0.001) {
					to_target = to_target.normalized();
					double angle_rad = query_forward.angle_to(to_target);

					if (angle_rad > half_angle_rad) {
						continue;
					}
				}
			}
		}

		// 可选去重检查
		if (p_processed_ids && p_processed_ids->has(target_id)) {
			continue;
		}

		if (p_processed_ids) {
			p_processed_ids->insert(target_id);
		}

		// 调用Lua回调函数
		lua_pushvalue(p_L, p_callback_index);
		lua_pushinteger(p_L, target_id);

		if (lua_pcall(p_L, 1, 1, 0) != LUA_OK) {
			godot::UtilityFunctions::printerr("native_collision: callback error: ", lua_tostring(p_L, -1));
			lua_pop(p_L, 1);
			return false;
		}

		if (lua_isboolean(p_L, -1) && !lua_toboolean(p_L, -1)) {
			lua_pop(p_L, 1);
			return false;
		}
		lua_pop(p_L, 1);
	}

	return true;
}

// 圆柱检测底层实现
static bool _intersect_cylinder_impl(
		lua_State *p_L,
		godot::Node3D *p_ref_node,
		const godot::Transform3D &p_transform,
		double p_radius,
		double p_height,
		double p_angle,
		uint32_t p_collision_mask,
		int p_callback_index,
		godot::RBSet<uint64_t> *p_processed_ids) {

	cached_cylinder_shape->set_radius(p_radius);
	cached_cylinder_shape->set_height(p_height);

	bool is_sector = (p_angle < 360.0);
	return _perform_shape_query(
			p_L, p_ref_node, cached_cylinder_shape,
			p_transform, p_collision_mask,
			is_sector, p_angle,
			p_callback_index, p_processed_ids);
}

// 立方体检测底层实现
static bool _intersect_box_impl(
		lua_State *p_L,
		godot::Node3D *p_ref_node,
		const godot::Transform3D &p_transform,
		const godot::Vector3 &p_size,
		uint32_t p_collision_mask,
		int p_callback_index,
		godot::RBSet<uint64_t> *p_processed_ids) {

	cached_box_shape->set_size(p_size);

	return _perform_shape_query(
			p_L, p_ref_node, cached_box_shape,
			p_transform, p_collision_mask,
			false, 0.0,
			p_callback_index, p_processed_ids);
}

// 处理单个hitbox的碰撞检测
static bool _process_single_hitbox(
		lua_State *p_L,
		godot::Node3D *p_hitbox_node,
		uint32_t p_collision_mask,
		int p_callback_index,
		godot::RBSet<uint64_t> *p_processed_ids) {

	godot::Variant shape_type_var = p_hitbox_node->get("shape_type");
	int shape_type = (int)shape_type_var;
	godot::Transform3D transform = p_hitbox_node->get_global_transform();

	if (shape_type == 0) { // CYLINDER
		double radius = (double)p_hitbox_node->get("cylinder_radius");
		double height = (double)p_hitbox_node->get("cylinder_height");
		double angle = (double)p_hitbox_node->get("cylinder_angle");

		return _intersect_cylinder_impl(
				p_L, p_hitbox_node, transform,
				radius, height, angle,
				p_collision_mask, p_callback_index, p_processed_ids);
	} else { // BOX
		godot::Vector3 size = (godot::Vector3)p_hitbox_node->get("box_size");

		return _intersect_box_impl(
				p_L, p_hitbox_node, transform, size,
				p_collision_mask, p_callback_index, p_processed_ids);
	}
}

// intersect_cylinder(ref_node_id, pos_x, pos_y, pos_z,
//                    forward_x, forward_y, forward_z,
//                    radius, height, angle,
//                    collision_mask, callback) -> void
// 对指定位置执行圆柱/扇柱空间检测，不使用AttackHitbox3D节点
// forward为零向量时报错
static int l_intersect_cylinder(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 12) {
		godot::UtilityFunctions::printerr("native_collision.intersect_cylinder: expected 12 args, got ", argc);
		return 0;
	}

	godot::ObjectID node_id = _read_node_id(p_L, 1);

	godot::Vector3 position(
			(float)luaL_checknumber(p_L, 2),
			(float)luaL_checknumber(p_L, 3),
			(float)luaL_checknumber(p_L, 4));
	godot::Vector3 forward(
			(float)luaL_checknumber(p_L, 5),
			(float)luaL_checknumber(p_L, 6),
			(float)luaL_checknumber(p_L, 7));

	if (forward.length_squared() < 0.001) {
		godot::UtilityFunctions::printerr("native_collision.intersect_cylinder: forward vector is zero");
		return 0;
	}

	forward.normalize();

	double radius = luaL_checknumber(p_L, 8);
	double height = luaL_checknumber(p_L, 9);
	double angle = luaL_checknumber(p_L, 10);
	uint32_t collision_mask = (uint32_t)luaL_checkinteger(p_L, 11);

	if (collision_mask == 0) {
		collision_mask = 0xFFFFFFFF;
	}

	luaL_checktype(p_L, 12, LUA_TFUNCTION);

	godot::Node3D *node = _resolve_node(node_id, "intersect_cylinder");
	if (!node) {
		return 0;
	}

	if (cached_cylinder_shape.is_null()) {
		cached_cylinder_shape.instantiate();
		cached_box_shape.instantiate();
		cached_query_params.instantiate();
	}

	// looking_at 内部将 -Z 指向 target，因此取反使 +Z 指向 forward
	godot::Basis basis = godot::Basis::looking_at(-forward, godot::Vector3(0, 1, 0));
	godot::Transform3D transform(basis, position);

	_intersect_cylinder_impl(
			p_L, node, transform,
			radius, height, angle,
			collision_mask, 12, nullptr);

	return 0;
}

// intersect_box(ref_node_id, pos_x, pos_y, pos_z,
//               rot_x, rot_y, rot_z,
//               size_x, size_y, size_z,
//               collision_mask, callback) -> void
// 对指定位置执行立方体空间检测，不使用AttackHitbox3D节点
static int l_intersect_box(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 12) {
		godot::UtilityFunctions::printerr("native_collision.intersect_box: expected 12 args, got ", argc);
		return 0;
	}

	godot::ObjectID node_id = _read_node_id(p_L, 1);

	godot::Vector3 position(
			(float)luaL_checknumber(p_L, 2),
			(float)luaL_checknumber(p_L, 3),
			(float)luaL_checknumber(p_L, 4));
	godot::Vector3 euler(
			(float)luaL_checknumber(p_L, 5),
			(float)luaL_checknumber(p_L, 6),
			(float)luaL_checknumber(p_L, 7));

	godot::Vector3 size(
			(float)luaL_checknumber(p_L, 8),
			(float)luaL_checknumber(p_L, 9),
			(float)luaL_checknumber(p_L, 10));
	uint32_t collision_mask = (uint32_t)luaL_checkinteger(p_L, 11);

	if (collision_mask == 0) {
		collision_mask = 0xFFFFFFFF;
	}

	luaL_checktype(p_L, 12, LUA_TFUNCTION);

	godot::Node3D *node = _resolve_node(node_id, "intersect_box");
	if (!node) {
		return 0;
	}

	if (cached_box_shape.is_null()) {
		cached_cylinder_shape.instantiate();
		cached_box_shape.instantiate();
		cached_query_params.instantiate();
	}

	godot::Basis basis = godot::Basis::from_euler(euler, godot::EulerOrder::EULER_ORDER_YXZ);
	godot::Transform3D transform(basis, position);

	_intersect_box_impl(
			p_L, node, transform, size,
			collision_mask, 12, nullptr);

	return 0;
}

// intersect_hitbox(node_id, collision_mask, callback) -> void
// 对指定节点或其子节点中的AttackHitbox3D执行碰撞检测
// 如果node_id本身是AttackHitbox3D，直接处理
// 否则遍历其直接子节点，找到所有AttackHitbox3D进行处理
// 对每个碰撞目标调用callback(target_id)，多个hitbox检测到同一目标只回调一次
// callback返回false可提前终止迭代
static int l_intersect_hitbox(lua_State *p_L) {
	// 延迟初始化Shape资源（第一次调用时）
	if (cached_cylinder_shape.is_null()) {
		cached_cylinder_shape.instantiate();
		cached_box_shape.instantiate();
		cached_query_params.instantiate();
	}

	// 1. 参数校验
	int argc = lua_gettop(p_L);
	if (argc < 3) {
		godot::UtilityFunctions::printerr("native_collision.intersect_hitbox: expected 3 args (node_id, collision_mask, callback), got ", argc);
		return 0;
	}

	// 2. 解析参数
	godot::ObjectID node_id = _read_node_id(p_L, 1);
	uint32_t collision_mask = (uint32_t)luaL_checkinteger(p_L, 2);
	luaL_checktype(p_L, 3, LUA_TFUNCTION);

	// 默认collision_mask为全层
	if (collision_mask == 0) {
		collision_mask = 0xFFFFFFFF;
	}

	// 3. 解析节点
	godot::Node3D *node = _resolve_node(node_id, "intersect_hitbox");
	if (!node) {
		return 0;
	}

	// 4. 收集所有需要处理的AttackHitbox3D节点
	godot::Vector<godot::Node3D *> hitboxes;

	// 检查node本身是否为AttackHitbox3D
	if (_is_attack_hitbox(node)) {
		hitboxes.push_back(node);
	} else {
		// 遍历直接子节点（仅一层）
		_collect_attack_hitboxes(node, hitboxes);
	}

	// 如果没有找到任何AttackHitbox3D，静默返回
	if (hitboxes.is_empty()) {
		return 0;
	}

	// 5. 创建去重集合
	godot::RBSet<uint64_t> processed_ids;

	// 6. 处理每个hitbox
	for (int i = 0; i < hitboxes.size(); i++) {
		bool should_continue = _process_single_hitbox(
				p_L,
				hitboxes[i],
				collision_mask,
				3, // 回调函数在栈索引3
				&processed_ids);

		if (!should_continue) {
			break; // 回调返回false或出错，提前终止
		}
	}

	return 0;
}

// 设置单个 AttackHitbox3D 的 active 属性
static void _set_single_hitbox_active(godot::Node3D *p_hitbox_node, bool p_active) {
	if (!p_hitbox_node) {
		return;
	}

	p_hitbox_node->set("active", p_active);
}

// set_hitbox_active(node_id, active) -> void
// 设置指定节点或其子节点中的 AttackHitbox3D 的 active 状态
// 如果 node_id 本身是 AttackHitbox3D，直接设置
// 否则遍历其直接子节点，找到所有 AttackHitbox3D 进行批量设置
static int l_set_hitbox_active(lua_State *p_L) {
	// 1. 参数校验
	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_collision.set_hitbox_active: expected 2 args (node_id, active), got ", argc);
		return 0;
	}

	// 2. 解析参数
	godot::ObjectID node_id = _read_node_id(p_L, 1);
	bool active = lua_toboolean(p_L, 2);

	// 3. 解析节点
	godot::Node3D *node = _resolve_node(node_id, "set_hitbox_active");
	if (!node) {
		return 0;
	}

	// 4. 收集所有需要处理的 AttackHitbox3D 节点
	godot::Vector<godot::Node3D *> hitboxes;

	// 检查 node 本身是否为 AttackHitbox3D
	if (_is_attack_hitbox(node)) {
		hitboxes.push_back(node);
	} else {
		// 遍历直接子节点（仅一层）
		_collect_attack_hitboxes(node, hitboxes);
	}

	// 如果没有找到任何 AttackHitbox3D，静默返回
	if (hitboxes.is_empty()) {
		return 0;
	}

	// 5. 设置每个 hitbox 的 active 状态
	for (int i = 0; i < hitboxes.size(); i++) {
		_set_single_hitbox_active(hitboxes[i], active);
	}

	return 0;
}

static const luaL_Reg collision_funcs[] = {
	{"get_aabb", l_get_aabb},
	{"intersect_hitbox", l_intersect_hitbox},
	{"set_hitbox_active", l_set_hitbox_active},
	{"intersect_cylinder", l_intersect_cylinder},
	{"intersect_box", l_intersect_box},
	{nullptr, nullptr}
};

int luaopen_native_collision(lua_State *p_L) {
	luaL_newlib(p_L, collision_funcs);
	return 1;
}

} // namespace luagd
