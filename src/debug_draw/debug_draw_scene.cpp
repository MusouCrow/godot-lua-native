#include "debug_draw_types.h"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace luagd {

// 固定调试绘制节点名。
static const char *DEBUG_DRAW_ROOT_NAME = "debug_draw_3d";
static const char *BUCKET_NAMES[DEBUG_BUCKET_COUNT] = {
	"points_depth",
	"points_xray",
	"lines_depth",
	"lines_xray",
	"faces_depth",
	"faces_xray"
};

godot::Node3D *debug_draw_resolve_node3d(godot::ObjectID p_id) {
	if (p_id.is_null()) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::Node3D>(godot::ObjectDB::get_instance((uint64_t)p_id));
}

// 通过缓存的 ObjectID 解析 bucket 节点。
static godot::MeshInstance3D *_resolve_bucket_instance(const DebugBucket &p_bucket) {
	if (p_bucket.instance_id.is_null()) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::MeshInstance3D>(godot::ObjectDB::get_instance((uint64_t)p_bucket.instance_id));
}

// 配置 bucket 的通用材质。
// 注意：xray 通过关闭深度测试实现穿墙显示。
static void _configure_material(const godot::Ref<godot::StandardMaterial3D> &p_material, bool p_is_xray) {
	if (p_material.is_null()) {
		return;
	}

	p_material->set_shading_mode(godot::BaseMaterial3D::SHADING_MODE_UNSHADED);
	p_material->set_transparency(godot::BaseMaterial3D::TRANSPARENCY_ALPHA);
	p_material->set_cull_mode(godot::BaseMaterial3D::CULL_DISABLED);
	p_material->set_flag(godot::BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	p_material->set_flag(godot::BaseMaterial3D::FLAG_DISABLE_FOG, true);
	p_material->set_flag(godot::BaseMaterial3D::FLAG_DISABLE_DEPTH_TEST, p_is_xray);
}

// 重置 bucket 运行时记录。
static void _reset_bucket(DebugBucket *p_bucket, DebugBucketKind p_kind) {
	if (p_bucket == nullptr) {
		return;
	}

	p_bucket->kind = p_kind;
	p_bucket->instance_id = godot::ObjectID();
	p_bucket->mesh.unref();
	p_bucket->material.unref();
	p_bucket->buffers.clear();
}

// 重置场景节点和资源记录。
void debug_draw_reset_scene(bool p_destroy_node) {
	DebugDrawState &state = debug_draw_get_state();
	if (p_destroy_node) {
		godot::Node3D *debug_root = debug_draw_resolve_node3d(state.debug_root_id);
		if (debug_root != nullptr && debug_root->is_inside_tree()) {
			debug_root->queue_free();
		}
	}

	state.debug_root_id = godot::ObjectID();
	for (int32_t i = 0; i < DEBUG_BUCKET_COUNT; ++i) {
		_reset_bucket(&state.buckets[i], (DebugBucketKind)i);
	}
}

// 同步调试绘制节点可见性。
void debug_draw_update_visibility() {
	DebugDrawState &state = debug_draw_get_state();
	godot::Node3D *debug_root = debug_draw_resolve_node3d(state.debug_root_id);
	if (debug_root == nullptr) {
		return;
	}

	debug_root->set_visible(state.enabled && state.visible);
}

// 清空已生成的 mesh 数据，但保留节点和资源对象。
void debug_draw_clear_meshes() {
	DebugDrawState &state = debug_draw_get_state();
	for (int32_t i = 0; i < DEBUG_BUCKET_COUNT; ++i) {
		DebugBucket *bucket = &state.buckets[i];
		bucket->buffers.clear();
		if (bucket->mesh.is_valid()) {
			bucket->mesh->clear_surfaces();
		}
	}

	state.stats.points_vertex_count = 0;
	state.stats.lines_vertex_count = 0;
	state.stats.faces_vertex_count = 0;
}

// 确保指定 bucket 的节点、mesh 和材质都已创建。
static bool _ensure_bucket(DebugDrawState &p_state, godot::Node3D *p_debug_root, DebugBucketKind p_kind) {
	DebugBucket *bucket = &p_state.buckets[p_kind];
	bucket->kind = p_kind;

	if (bucket->mesh.is_null()) {
		bucket->mesh.instantiate();
	}
	if (bucket->material.is_null()) {
		bucket->material.instantiate();
		_configure_material(bucket->material, p_kind == DEBUG_BUCKET_POINTS_XRAY || p_kind == DEBUG_BUCKET_LINES_XRAY || p_kind == DEBUG_BUCKET_FACES_XRAY);
	}

	godot::MeshInstance3D *instance = _resolve_bucket_instance(*bucket);
	if (instance == nullptr) {
		godot::Node *existing = p_debug_root->get_node_or_null(godot::NodePath(BUCKET_NAMES[p_kind]));
		if (existing != nullptr) {
			instance = godot::Object::cast_to<godot::MeshInstance3D>(existing);
			if (instance == nullptr) {
				godot::UtilityFunctions::printerr("native_debug_draw: child exists but is not MeshInstance3D: ", BUCKET_NAMES[p_kind]);
				return false;
			}
		} else {
			instance = memnew(godot::MeshInstance3D);
			instance->set_name(BUCKET_NAMES[p_kind]);
			p_debug_root->add_child(instance);
		}

		bucket->instance_id = godot::ObjectID(instance->get_instance_id());
	}

	instance->set_mesh(bucket->mesh);
	instance->set_material_override(bucket->material);
	return true;
}

// 确保调试绘制节点树和 6 个 bucket 已创建完成。
bool debug_draw_ensure_scene() {
	DebugDrawState &state = debug_draw_get_state();
	if (state.root_node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_debug_draw: root not set, call set_root first");
		return false;
	}

	godot::Node3D *root_node = debug_draw_resolve_node3d(state.root_node_id);
	if (root_node == nullptr || !root_node->is_inside_tree()) {
		godot::UtilityFunctions::printerr("native_debug_draw: root node is no longer valid");
		debug_draw_reset_scene(false);
		state.root_node_id = godot::ObjectID();
		return false;
	}

	godot::Node3D *debug_root = debug_draw_resolve_node3d(state.debug_root_id);
	if (debug_root == nullptr) {
		godot::Node *existing = root_node->get_node_or_null(godot::NodePath(DEBUG_DRAW_ROOT_NAME));
		if (existing != nullptr) {
			debug_root = godot::Object::cast_to<godot::Node3D>(existing);
			if (debug_root == nullptr) {
				godot::UtilityFunctions::printerr("native_debug_draw: child exists but is not Node3D: ", DEBUG_DRAW_ROOT_NAME);
				return false;
			}
		} else {
			debug_root = memnew(godot::Node3D);
			debug_root->set_name(DEBUG_DRAW_ROOT_NAME);
			root_node->add_child(debug_root);
		}

		state.debug_root_id = godot::ObjectID(debug_root->get_instance_id());
	}

	for (int32_t i = 0; i < DEBUG_BUCKET_COUNT; ++i) {
		if (!_ensure_bucket(state, debug_root, (DebugBucketKind)i)) {
			return false;
		}
	}

	debug_draw_update_visibility();
	return true;
}

} // namespace luagd
