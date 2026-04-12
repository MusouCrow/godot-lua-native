#ifndef LUAGD_DEBUG_DRAW_MODULE_TYPES_H
#define LUAGD_DEBUG_DRAW_MODULE_TYPES_H

#include <cstdint>

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace luagd {

// 渲染 bucket 分类。
enum DebugBucketKind {
	DEBUG_BUCKET_POINTS_DEPTH = 0,
	DEBUG_BUCKET_POINTS_XRAY = 1,
	DEBUG_BUCKET_LINES_DEPTH = 2,
	DEBUG_BUCKET_LINES_XRAY = 3,
	DEBUG_BUCKET_FACES_DEPTH = 4,
	DEBUG_BUCKET_FACES_XRAY = 5,
	DEBUG_BUCKET_COUNT = 6
};

struct DebugBucketBuffers {
	godot::PackedVector3Array positions;
	godot::PackedColorArray colors;
	int32_t vertex_count;

	DebugBucketBuffers() : vertex_count(0) {}

	void clear() {
		positions.clear();
		colors.clear();
		vertex_count = 0;
	}

	void append(const godot::Vector3 &p_position, const godot::Color &p_color) {
		positions.push_back(p_position);
		colors.push_back(p_color);
		vertex_count += 1;
	}
};

struct DebugBucket {
	DebugBucketKind kind;
	godot::ObjectID instance_id;
	godot::Ref<godot::ArrayMesh> mesh;
	godot::Ref<godot::StandardMaterial3D> material;
	DebugBucketBuffers buffers;

	DebugBucket() : kind(DEBUG_BUCKET_POINTS_DEPTH) {}
};

struct DebugPointCommand {
	godot::Vector3 position;
	godot::Color color;
	float size;
	bool is_xray;
};

struct DebugLineCommand {
	godot::Vector3 from;
	godot::Vector3 to;
	godot::Color color;
	float width;
	bool is_xray;
};

struct DebugCircleCommand {
	godot::Vector3 center;
	godot::Vector3 normal;
	godot::Color color;
	float radius;
	float line_width;
	int32_t segments;
	bool is_xray;
	bool is_fill;
};

struct DebugSectorCommand {
	godot::Vector3 center;
	godot::Vector3 normal;
	godot::Vector3 direction;
	godot::Color color;
	float radius;
	float angle_degrees;
	float line_width;
	int32_t segments;
	bool is_xray;
	bool is_fill;
};

struct DebugFrameStats {
	int32_t point_commands;
	int32_t line_commands;
	int32_t circle_commands;
	int32_t sector_commands;
	int32_t points_vertex_count;
	int32_t lines_vertex_count;
	int32_t faces_vertex_count;
	int32_t submit_count;
	bool had_camera;

	DebugFrameStats() {
		reset();
	}

	void reset() {
		point_commands = 0;
		line_commands = 0;
		circle_commands = 0;
		sector_commands = 0;
		points_vertex_count = 0;
		lines_vertex_count = 0;
		faces_vertex_count = 0;
		submit_count = 0;
		had_camera = false;
	}
};

struct DebugScratchCache {
	godot::Vector<godot::Vector3> sampled_points;

	void clear() {
		sampled_points.clear();
	}
};

struct DebugBuildContext {
	godot::Camera3D *camera;
	godot::Vector3 camera_position;
	godot::Vector3 camera_right;
	godot::Vector3 camera_up;
	bool has_camera;

	DebugBuildContext() : camera(nullptr), has_camera(false) {}
};

struct DebugDrawState {
	bool enabled;
	bool visible;
	godot::ObjectID root_node_id;
	godot::ObjectID debug_root_id;
	DebugBucket buckets[DEBUG_BUCKET_COUNT];
	godot::Vector<DebugPointCommand> point_commands;
	godot::Vector<DebugLineCommand> line_commands;
	godot::Vector<DebugCircleCommand> circle_commands;
	godot::Vector<DebugSectorCommand> sector_commands;
	DebugFrameStats stats;
	DebugScratchCache scratch;

	DebugDrawState() : enabled(true), visible(true) {}
};

DebugDrawState &debug_draw_get_state();
godot::Node3D *debug_draw_resolve_node3d(godot::ObjectID p_id);
void debug_draw_reset_scene(bool p_destroy_node);
bool debug_draw_ensure_scene();
void debug_draw_update_visibility();
void debug_draw_clear_meshes();
void debug_draw_clear_commands();
bool debug_draw_build_and_submit();

} // namespace luagd

#endif // LUAGD_DEBUG_DRAW_MODULE_TYPES_H
