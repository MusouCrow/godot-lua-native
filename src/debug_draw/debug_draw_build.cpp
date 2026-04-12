#include "debug_draw_types.h"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace luagd {

static const float EPSILON_LENGTH_SQUARED = 0.000001f;

// 安全归一化，零向量时回退到备用方向。
static godot::Vector3 _safe_normalized(const godot::Vector3 &p_value, const godot::Vector3 &p_fallback) {
	if (p_value.length_squared() <= EPSILON_LENGTH_SQUARED) {
		return p_fallback;
	}
	return p_value.normalized();
}

// 追加一个三角形。
static void _append_triangle(DebugBucketBuffers *p_buffers, const godot::Vector3 &p_a, const godot::Vector3 &p_b, const godot::Vector3 &p_c, const godot::Color &p_color) {
	p_buffers->append(p_a, p_color);
	p_buffers->append(p_b, p_color);
	p_buffers->append(p_c, p_color);
}

// 追加一个 quad，内部拆成两个三角形。
static void _append_quad(DebugBucketBuffers *p_buffers, const godot::Vector3 &p_a, const godot::Vector3 &p_b, const godot::Vector3 &p_c, const godot::Vector3 &p_d, const godot::Color &p_color) {
	_append_triangle(p_buffers, p_a, p_b, p_c, p_color);
	_append_triangle(p_buffers, p_a, p_c, p_d, p_color);
}

// 基于法线构建圆面所在平面的两个正交轴。
static void _build_plane_basis(const godot::Vector3 &p_normal, godot::Vector3 *r_tangent, godot::Vector3 *r_bitangent) {
	const godot::Vector3 normal = _safe_normalized(p_normal, godot::Vector3(0.0f, 1.0f, 0.0f));
	const godot::Vector3 seed = godot::Math::abs(normal.dot(godot::Vector3(0.0f, 1.0f, 0.0f))) > 0.99f
		? godot::Vector3(1.0f, 0.0f, 0.0f)
		: godot::Vector3(0.0f, 1.0f, 0.0f);
	const godot::Vector3 tangent = _safe_normalized(seed.cross(normal), godot::Vector3(1.0f, 0.0f, 0.0f));
	const godot::Vector3 bitangent = _safe_normalized(normal.cross(tangent), godot::Vector3(0.0f, 0.0f, 1.0f));

	*r_tangent = tangent;
	*r_bitangent = bitangent;
}

// 将点命令构建为面向相机的 quad。
static void _append_billboard_point(DebugBucketBuffers *p_buffers, const DebugPointCommand &p_command, const DebugBuildContext &p_context) {
	const godot::Vector3 right = p_context.has_camera ? p_context.camera_right : godot::Vector3(1.0f, 0.0f, 0.0f);
	const godot::Vector3 up = p_context.has_camera ? p_context.camera_up : godot::Vector3(0.0f, 1.0f, 0.0f);
	const float half_size = p_command.size * 0.5f;
	const godot::Vector3 offset_right = right * half_size;
	const godot::Vector3 offset_up = up * half_size;

	const godot::Vector3 a = p_command.position - offset_right - offset_up;
	const godot::Vector3 b = p_command.position + offset_right - offset_up;
	const godot::Vector3 c = p_command.position + offset_right + offset_up;
	const godot::Vector3 d = p_command.position - offset_right + offset_up;
	_append_quad(p_buffers, a, b, c, d, p_command.color);
}

// 将线命令构建为 ribbon quad。
static void _append_line_quad(DebugBucketBuffers *p_buffers, const godot::Vector3 &p_from, const godot::Vector3 &p_to, float p_width, const godot::Color &p_color, const DebugBuildContext &p_context) {
	const godot::Vector3 direction = p_to - p_from;
	if (direction.length_squared() <= EPSILON_LENGTH_SQUARED) {
		return;
	}

	const godot::Vector3 line_dir = direction.normalized();
	const godot::Vector3 midpoint = (p_from + p_to) * 0.5f;
	const godot::Vector3 fallback_view_dir = godot::Vector3(0.0f, 0.0f, -1.0f);
	const godot::Vector3 view_dir = p_context.has_camera
		? _safe_normalized(p_context.camera_position - midpoint, fallback_view_dir)
		: fallback_view_dir;

	godot::Vector3 side = line_dir.cross(view_dir);
	if (side.length_squared() <= EPSILON_LENGTH_SQUARED) {
		side = line_dir.cross(godot::Vector3(0.0f, 1.0f, 0.0f));
	}
	if (side.length_squared() <= EPSILON_LENGTH_SQUARED) {
		side = line_dir.cross(godot::Vector3(1.0f, 0.0f, 0.0f));
	}
	side = _safe_normalized(side, godot::Vector3(1.0f, 0.0f, 0.0f)) * (p_width * 0.5f);

	const godot::Vector3 a = p_from - side;
	const godot::Vector3 b = p_from + side;
	const godot::Vector3 c = p_to + side;
	const godot::Vector3 d = p_to - side;
	_append_quad(p_buffers, a, b, c, d, p_color);
}

// 采样圆周点。
static void _sample_circle_points(const godot::Vector3 &p_center, const godot::Vector3 &p_normal, float p_radius, int32_t p_segments, godot::Vector<godot::Vector3> *r_points) {
	r_points->clear();
	r_points->resize(p_segments + 1);

	godot::Vector3 tangent;
	godot::Vector3 bitangent;
	_build_plane_basis(p_normal, &tangent, &bitangent);

	for (int32_t i = 0; i <= p_segments; ++i) {
		const float ratio = (float)i / (float)p_segments;
		const float angle = ratio * Math_TAU;
		const godot::Vector3 offset = tangent * godot::Math::cos(angle) * p_radius + bitangent * godot::Math::sin(angle) * p_radius;
		(*r_points).write[i] = p_center + offset;
	}
}

// 采样扇形弧线点。
static void _sample_sector_points(const DebugSectorCommand &p_command, godot::Vector<godot::Vector3> *r_points) {
	r_points->clear();
	r_points->resize(p_command.segments + 1);

	const godot::Vector3 normal = _safe_normalized(p_command.normal, godot::Vector3(0.0f, 1.0f, 0.0f));
	const godot::Vector3 projected_dir = p_command.direction - normal * normal.dot(p_command.direction);
	const godot::Vector3 start_dir = _safe_normalized(projected_dir, godot::Vector3(1.0f, 0.0f, 0.0f));
	const godot::Vector3 side_dir = _safe_normalized(normal.cross(start_dir), godot::Vector3(0.0f, 0.0f, 1.0f));
	const float half_angle = godot::Math::deg_to_rad(p_command.angle_degrees) * 0.5f;

	for (int32_t i = 0; i <= p_command.segments; ++i) {
		const float ratio = (float)i / (float)p_command.segments;
		const float angle = -half_angle + ratio * half_angle * 2.0f;
		const godot::Vector3 dir = (start_dir * godot::Math::cos(angle) + side_dir * godot::Math::sin(angle)).normalized();
		(*r_points).write[i] = p_command.center + dir * p_command.radius;
	}
}

// 构建圆命令对应的线框或面片。
static void _append_circle(DebugDrawState &p_state, const DebugCircleCommand &p_command, const DebugBuildContext &p_context) {
	DebugBucket *face_bucket = &p_state.buckets[p_command.is_xray ? DEBUG_BUCKET_FACES_XRAY : DEBUG_BUCKET_FACES_DEPTH];
	DebugBucket *line_bucket = &p_state.buckets[p_command.is_xray ? DEBUG_BUCKET_LINES_XRAY : DEBUG_BUCKET_LINES_DEPTH];

	_sample_circle_points(p_command.center, p_command.normal, p_command.radius, p_command.segments, &p_state.scratch.sampled_points);
	if (p_command.is_fill) {
		for (int32_t i = 0; i < p_command.segments; ++i) {
			_append_triangle(&face_bucket->buffers, p_command.center, p_state.scratch.sampled_points[i], p_state.scratch.sampled_points[i + 1], p_command.color);
		}
	} else {
		for (int32_t i = 0; i < p_command.segments; ++i) {
			_append_line_quad(&line_bucket->buffers, p_state.scratch.sampled_points[i], p_state.scratch.sampled_points[i + 1], p_command.line_width, p_command.color, p_context);
		}
	}
}

// 构建扇形命令对应的线框或面片。
static void _append_sector(DebugDrawState &p_state, const DebugSectorCommand &p_command, const DebugBuildContext &p_context) {
	DebugBucket *face_bucket = &p_state.buckets[p_command.is_xray ? DEBUG_BUCKET_FACES_XRAY : DEBUG_BUCKET_FACES_DEPTH];
	DebugBucket *line_bucket = &p_state.buckets[p_command.is_xray ? DEBUG_BUCKET_LINES_XRAY : DEBUG_BUCKET_LINES_DEPTH];

	_sample_sector_points(p_command, &p_state.scratch.sampled_points);
	if (p_command.is_fill) {
		for (int32_t i = 0; i < p_command.segments; ++i) {
			_append_triangle(&face_bucket->buffers, p_command.center, p_state.scratch.sampled_points[i], p_state.scratch.sampled_points[i + 1], p_command.color);
		}
	} else {
		for (int32_t i = 0; i < p_command.segments; ++i) {
			_append_line_quad(&line_bucket->buffers, p_state.scratch.sampled_points[i], p_state.scratch.sampled_points[i + 1], p_command.line_width, p_command.color, p_context);
		}
		_append_line_quad(&line_bucket->buffers, p_command.center, p_state.scratch.sampled_points[0], p_command.line_width, p_command.color, p_context);
		_append_line_quad(&line_bucket->buffers, p_command.center, p_state.scratch.sampled_points[p_command.segments], p_command.line_width, p_command.color, p_context);
	}
}

// 将 bucket 缓冲提交到 ArrayMesh。
static void _commit_bucket(DebugBucket *p_bucket) {
	if (p_bucket->mesh.is_null()) {
		return;
	}

	p_bucket->mesh->clear_surfaces();
	if (p_bucket->buffers.vertex_count <= 0) {
		return;
	}

	godot::Array arrays;
	arrays.resize(godot::Mesh::ARRAY_MAX);
	arrays[godot::Mesh::ARRAY_VERTEX] = p_bucket->buffers.positions;
	arrays[godot::Mesh::ARRAY_COLOR] = p_bucket->buffers.colors;
	p_bucket->mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_TRIANGLES, arrays);
}

// 构建并提交所有调试绘制命令。
bool debug_draw_build_and_submit() {
	DebugDrawState &state = debug_draw_get_state();
	if (!debug_draw_ensure_scene()) {
		return false;
	}

	for (int32_t i = 0; i < DEBUG_BUCKET_COUNT; ++i) {
		state.buckets[i].buffers.clear();
	}

	DebugBuildContext context;
	context.camera = nullptr;
	context.has_camera = false;

	godot::Node3D *debug_root = debug_draw_resolve_node3d(state.debug_root_id);
	if (debug_root != nullptr) {
		godot::Viewport *viewport = debug_root->get_viewport();
		if (viewport != nullptr) {
			context.camera = viewport->get_camera_3d();
		}
	}
	if (context.camera != nullptr) {
		const godot::Transform3D transform = context.camera->get_global_transform();
		context.camera_position = transform.origin;
		context.camera_right = transform.basis.get_column(0).normalized();
		context.camera_up = transform.basis.get_column(1).normalized();
		context.has_camera = true;
	} else {
		context.camera_position = godot::Vector3();
		context.camera_right = godot::Vector3(1.0f, 0.0f, 0.0f);
		context.camera_up = godot::Vector3(0.0f, 1.0f, 0.0f);
	}

	for (int32_t i = 0; i < state.point_commands.size(); ++i) {
		const DebugPointCommand &command = state.point_commands[i];
		DebugBucket *bucket = &state.buckets[command.is_xray ? DEBUG_BUCKET_POINTS_XRAY : DEBUG_BUCKET_POINTS_DEPTH];
		_append_billboard_point(&bucket->buffers, command, context);
	}

	for (int32_t i = 0; i < state.line_commands.size(); ++i) {
		const DebugLineCommand &command = state.line_commands[i];
		DebugBucket *bucket = &state.buckets[command.is_xray ? DEBUG_BUCKET_LINES_XRAY : DEBUG_BUCKET_LINES_DEPTH];
		_append_line_quad(&bucket->buffers, command.from, command.to, command.width, command.color, context);
	}

	for (int32_t i = 0; i < state.circle_commands.size(); ++i) {
		_append_circle(state, state.circle_commands[i], context);
	}

	for (int32_t i = 0; i < state.sector_commands.size(); ++i) {
		_append_sector(state, state.sector_commands[i], context);
	}

	for (int32_t i = 0; i < DEBUG_BUCKET_COUNT; ++i) {
		_commit_bucket(&state.buckets[i]);
	}

	state.stats.point_commands = state.point_commands.size();
	state.stats.line_commands = state.line_commands.size();
	state.stats.circle_commands = state.circle_commands.size();
	state.stats.sector_commands = state.sector_commands.size();
	state.stats.points_vertex_count = state.buckets[DEBUG_BUCKET_POINTS_DEPTH].buffers.vertex_count + state.buckets[DEBUG_BUCKET_POINTS_XRAY].buffers.vertex_count;
	state.stats.lines_vertex_count = state.buckets[DEBUG_BUCKET_LINES_DEPTH].buffers.vertex_count + state.buckets[DEBUG_BUCKET_LINES_XRAY].buffers.vertex_count;
	state.stats.faces_vertex_count = state.buckets[DEBUG_BUCKET_FACES_DEPTH].buffers.vertex_count + state.buckets[DEBUG_BUCKET_FACES_XRAY].buffers.vertex_count;
	state.stats.submit_count += 1;
	state.stats.had_camera = context.has_camera;

	debug_draw_clear_commands();
	return true;
}

} // namespace luagd
