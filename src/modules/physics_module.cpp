#include "physics_module.h"

#include "node_module.h"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/character_body3d.hpp>
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
		godot::UtilityFunctions::printerr("native_physics.", p_func_name, ": node id is 0");
		return nullptr;
	}

	godot::Node3D *node = node_resolve(p_node_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_physics.", p_func_name, ": node is no longer valid, id ", p_node_id);
		return nullptr;
	}

	return node;
}

static godot::CharacterBody3D *_resolve_character_body(godot::ObjectID p_node_id, const char *p_func_name) {
	godot::Node3D *node = _resolve_node(p_node_id, p_func_name);
	if (node == nullptr) {
		return nullptr;
	}

	godot::CharacterBody3D *body = godot::Object::cast_to<godot::CharacterBody3D>(node);
	if (body == nullptr) {
		godot::UtilityFunctions::printerr("native_physics.", p_func_name, ": node is not CharacterBody3D, id ", p_node_id);
		return nullptr;
	}

	return body;
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

// move_and_slide(node_id) -> bool
// 执行移动并滑动。
static int l_move_and_slide(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "move_and_slide");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->move_and_slide());
	return 1;
}

// set_velocity(node_id, x, y, z) -> void
// 设置速度向量。
static int l_set_velocity(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);

	godot::CharacterBody3D *body = _resolve_character_body(node_id, "set_velocity");
	if (body == nullptr) {
		return 0;
	}

	body->set_velocity(godot::Vector3((float)x, (float)y, (float)z));
	return 0;
}

// get_velocity(node_id) -> x, y, z
// 获取速度向量。
static int l_get_velocity(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "get_velocity");
	if (body == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	const godot::Vector3 velocity = body->get_velocity();
	lua_pushnumber(p_L, velocity.x);
	lua_pushnumber(p_L, velocity.y);
	lua_pushnumber(p_L, velocity.z);
	return 3;
}

// get_real_velocity(node_id) -> x, y, z
// 获取实际移动速度。
static int l_get_real_velocity(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "get_real_velocity");
	if (body == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	const godot::Vector3 velocity = body->get_real_velocity();
	lua_pushnumber(p_L, velocity.x);
	lua_pushnumber(p_L, velocity.y);
	lua_pushnumber(p_L, velocity.z);
	return 3;
}

// is_on_floor(node_id) -> bool
// 检查是否在地面上。
static int l_is_on_floor(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "is_on_floor");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->is_on_floor());
	return 1;
}

// is_on_wall(node_id) -> bool
// 检查是否在墙上。
static int l_is_on_wall(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "is_on_wall");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->is_on_wall());
	return 1;
}

// is_on_ceiling(node_id) -> bool
// 检查是否在天花板上。
static int l_is_on_ceiling(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "is_on_ceiling");
	if (body == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, body->is_on_ceiling());
	return 1;
}

// get_floor_normal(node_id) -> x, y, z
// 获取地面法线。
static int l_get_floor_normal(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::CharacterBody3D *body = _resolve_character_body(node_id, "get_floor_normal");
	if (body == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	const godot::Vector3 normal = body->get_floor_normal();
	lua_pushnumber(p_L, normal.x);
	lua_pushnumber(p_L, normal.y);
	lua_pushnumber(p_L, normal.z);
	return 3;
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

// 处理单个hitbox的碰撞检测
static bool _process_single_hitbox(
		lua_State *p_L,
		godot::Node3D *p_hitbox_node,
		uint32_t p_collision_mask,
		int p_callback_index,
		godot::RBSet<uint64_t> &r_processed_ids) {
	// 1. 读取AttackHitbox3D属性
	godot::Variant shape_type_var = p_hitbox_node->get("shape_type");
	int shape_type = (int)shape_type_var;
	godot::Transform3D hitbox_transform = p_hitbox_node->get_global_transform();

	// 2. 配置Shape和查询参数
	godot::Ref<godot::Shape3D> shape;
	double cylinder_angle = 360.0;

	if (shape_type == 0) { // CYLINDER
		double radius = (double)p_hitbox_node->get("cylinder_radius");
		double height = (double)p_hitbox_node->get("cylinder_height");
		cylinder_angle = (double)p_hitbox_node->get("cylinder_angle");

		cached_cylinder_shape->set_radius(radius);
		cached_cylinder_shape->set_height(height);
		shape = cached_cylinder_shape;
	} else { // BOX
		godot::Vector3 size = (godot::Vector3)p_hitbox_node->get("box_size");
		cached_box_shape->set_size(size);
		shape = cached_box_shape;
	}

	cached_query_params->set_shape(shape);
	cached_query_params->set_transform(hitbox_transform);
	cached_query_params->set_collision_mask(p_collision_mask);

	// 3. 获取PhysicsDirectSpaceState3D并执行检测
	godot::Ref<godot::World3D> world = p_hitbox_node->get_world_3d();
	if (world.is_null()) {
		godot::UtilityFunctions::printerr("native_physics.intersect_hitbox: hitbox node not in world");
		return false; // 严肃处理：直接失败
	}

	godot::PhysicsDirectSpaceState3D *space_state = world->get_direct_space_state();
	if (!space_state) {
		godot::UtilityFunctions::printerr("native_physics.intersect_hitbox: failed to get space state");
		return false; // 严肃处理：直接失败
	}

	godot::TypedArray<godot::Dictionary> results = space_state->intersect_shape(cached_query_params);

	// 4. 处理扇柱角度过滤（如果需要）
	bool is_sector = (shape_type == 0 && cylinder_angle < 360.0);
	godot::Vector3 hitbox_pos;
	godot::Vector3 hitbox_forward;
	double half_angle_rad = 0.0;

	if (is_sector) {
		hitbox_pos = hitbox_transform.get_origin();
		hitbox_forward = hitbox_transform.basis.get_column(2); // +Z轴
		half_angle_rad = (cylinder_angle / 2.0) * Math_PI / 180.0;
	}

	// 5. 对每个结果进行去重和回调
	for (int i = 0; i < results.size(); i++) {
		godot::Dictionary result = results[i];
		uint64_t target_id = (uint64_t)result["collider_id"];

		// 扇柱角度过滤
		if (is_sector) {
			godot::Object *collider = (godot::Object *)result["collider"];
			godot::Node3D *target_node = godot::Object::cast_to<godot::Node3D>(collider);

			if (target_node) {
				godot::Vector3 target_pos = target_node->get_global_position();
				godot::Vector3 to_target = target_pos - hitbox_pos;
				to_target.y = 0; // 投影到XZ平面

				if (to_target.length_squared() > 0.001) { // 避免零向量
					to_target = to_target.normalized();
					double angle_rad = hitbox_forward.angle_to(to_target);

					if (angle_rad > half_angle_rad) {
						continue; // 不在扇形范围内，跳过
					}
				}
			}
		}

		// 去重检查
		if (r_processed_ids.has(target_id)) {
			continue; // 已处理过，跳过
		}

		// 添加到已处理集合
		r_processed_ids.insert(target_id);

		// 调用Lua回调函数
		lua_pushvalue(p_L, p_callback_index); // 复制回调函数到栈顶
		lua_pushinteger(p_L, target_id);

		if (lua_pcall(p_L, 1, 1, 0) != LUA_OK) {
			godot::UtilityFunctions::printerr("native_physics.intersect_hitbox: callback error: ", lua_tostring(p_L, -1));
			lua_pop(p_L, 1);
			return false; // 回调出错，停止处理
		}

		// 检查返回值，如果是false则提前终止
		if (lua_isboolean(p_L, -1) && !lua_toboolean(p_L, -1)) {
			lua_pop(p_L, 1);
			return false; // 回调要求停止
		}
		lua_pop(p_L, 1);
	}

	return true; // 继续处理下一个hitbox
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
		godot::UtilityFunctions::printerr("native_physics.intersect_hitbox: expected 3 args (node_id, collision_mask, callback), got ", argc);
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
				processed_ids);

		if (!should_continue) {
			break; // 回调返回false或出错，提前终止
		}
	}

	return 0;
}

static const luaL_Reg physics_funcs[] = {
	{"get_aabb", l_get_aabb},
	{"move_and_slide", l_move_and_slide},
	{"set_velocity", l_set_velocity},
	{"get_velocity", l_get_velocity},
	{"get_real_velocity", l_get_real_velocity},
	{"is_on_floor", l_is_on_floor},
	{"is_on_wall", l_is_on_wall},
	{"is_on_ceiling", l_is_on_ceiling},
	{"get_floor_normal", l_get_floor_normal},
	{"intersect_hitbox", l_intersect_hitbox},
	{nullptr, nullptr}
};

int luaopen_native_physics(lua_State *p_L) {
	luaL_newlib(p_L, physics_funcs);
	return 1;
}

} // namespace luagd
