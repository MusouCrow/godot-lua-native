#include "particles_module.h"

#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform3d.hpp>
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

static void _push_bool(lua_State *p_L, bool p_value) {
	lua_pushboolean(p_L, p_value);
}

static godot::GPUParticles3D *_resolve_particles(godot::ObjectID p_node_id, const char *p_func_name) {
	if (p_node_id.is_null()) {
		godot::UtilityFunctions::printerr("native_particles.", p_func_name, ": node id is 0");
		return nullptr;
	}

	godot::Object *object = godot::ObjectDB::get_instance((uint64_t)p_node_id);
	if (object == nullptr) {
		godot::UtilityFunctions::printerr("native_particles.", p_func_name, ": node is no longer valid, id ", p_node_id);
		return nullptr;
	}

	godot::GPUParticles3D *particles = godot::Object::cast_to<godot::GPUParticles3D>(object);
	if (particles == nullptr) {
		godot::UtilityFunctions::printerr("native_particles.", p_func_name, ": node is not GPUParticles3D, id ", p_node_id);
		return nullptr;
	}

	return particles;
}

// play(node_id) -> bool
// 开始发射粒子。
static int l_play(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::GPUParticles3D *particles = _resolve_particles(node_id, "play");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	particles->set_emitting(true);
	_push_bool(p_L, true);
	return 1;
}

// stop(node_id) -> bool
// 停止继续发射，不清空现有粒子。
static int l_stop(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::GPUParticles3D *particles = _resolve_particles(node_id, "stop");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	particles->set_emitting(false);
	_push_bool(p_L, true);
	return 1;
}

// clear(node_id) -> bool
// 清空现有粒子，同时保持调用前的播放状态。
static int l_clear(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::GPUParticles3D *particles = _resolve_particles(node_id, "clear");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	const bool was_playing = particles->is_emitting();
	particles->restart(false);
	if (!was_playing) {
		particles->set_emitting(false);
	}

	_push_bool(p_L, true);
	return 1;
}

// set_speed_scale(node_id, speed_scale) -> bool
// 设置粒子模拟速度倍率。
static int l_set_speed_scale(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double speed_scale = luaL_checknumber(p_L, 2);
	godot::GPUParticles3D *particles = _resolve_particles(node_id, "set_speed_scale");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	particles->set_speed_scale(speed_scale);
	_push_bool(p_L, true);
	return 1;
}

// is_playing(node_id) -> bool
// 查询当前是否仍在发射新粒子。
static int l_is_playing(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::GPUParticles3D *particles = _resolve_particles(node_id, "is_playing");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	_push_bool(p_L, particles->is_emitting());
	return 1;
}

// is_alive(node_id) -> bool
// 查询粒子系统是否仍处于活跃状态。
// 该接口依赖真实粒子渲染后端维护 inactive 状态；在 dummy/headless 后端下不保证结果可靠。
static int l_is_alive(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::GPUParticles3D *particles = _resolve_particles(node_id, "is_alive");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	const bool inactive = godot::RenderingServer::get_singleton()->particles_is_inactive(particles->get_base());
	_push_bool(p_L, !inactive);
	return 1;
}

// emit_particle(node_id, pos_x, pos_y, pos_z, vel_x?, vel_y?, vel_z?, r?, g?, b?, a?, custom_x?, custom_y?, custom_z?, custom_w?, rot_x?, rot_y?, rot_z?, scale_x?, scale_y?, scale_z?) -> bool
// 强制发射单个粒子。
// 位置必填；速度、颜色、custom、旋转(度数)、缩放均可省略，省略项不设置对应 EmitFlag，
// 该属性交还粒子材质自行随机取值。
// 注意：默认 ParticleProcessMaterial 下 custom 语义为 (rotation, age, animation, lifetime)。
// 注意：仅 Forward+ 与 Mobile 渲染方法支持该接口，Compatibility 与 dummy/headless 下为无效操作。
// 注意：调用后会关闭持续发射，需配合 play 重新开启。
static int l_emit_particle(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	godot::GPUParticles3D *particles = _resolve_particles(node_id, "emit_particle");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	// 约束：位置三轴必填。
	if (lua_isnoneornil(p_L, 2) || lua_isnoneornil(p_L, 3) || lua_isnoneornil(p_L, 4)) {
		_push_bool(p_L, false);
		return 1;
	}

	const godot::Vector3 position(
		(float)luaL_checknumber(p_L, 2),
		(float)luaL_checknumber(p_L, 3),
		(float)luaL_checknumber(p_L, 4)
	);

	uint32_t emit_flags = godot::RenderingServer::PARTICLES_EMIT_FLAG_POSITION;

	// 速度组：三轴任一提供则整体生效，缺失轴补默认值 0。
	const float vel_x = lua_isnoneornil(p_L, 5) ? 0.0f : (float)luaL_checknumber(p_L, 5);
	const float vel_y = lua_isnoneornil(p_L, 6) ? 0.0f : (float)luaL_checknumber(p_L, 6);
	const float vel_z = lua_isnoneornil(p_L, 7) ? 0.0f : (float)luaL_checknumber(p_L, 7);
	const godot::Vector3 velocity(vel_x, vel_y, vel_z);
	if (!lua_isnoneornil(p_L, 5) || !lua_isnoneornil(p_L, 6) || !lua_isnoneornil(p_L, 7)) {
		emit_flags |= godot::RenderingServer::PARTICLES_EMIT_FLAG_VELOCITY;
	}

	// 颜色：四分量任一提供则整体有效，缺失分量补默认值 1。
	const float color_r = lua_isnoneornil(p_L, 8) ? 1.0f : (float)luaL_checknumber(p_L, 8);
	const float color_g = lua_isnoneornil(p_L, 9) ? 1.0f : (float)luaL_checknumber(p_L, 9);
	const float color_b = lua_isnoneornil(p_L, 10) ? 1.0f : (float)luaL_checknumber(p_L, 10);
	const float color_a = lua_isnoneornil(p_L, 11) ? 1.0f : (float)luaL_checknumber(p_L, 11);
	const godot::Color color(color_r, color_g, color_b, color_a);
	if (!lua_isnoneornil(p_L, 8) || !lua_isnoneornil(p_L, 9) || !lua_isnoneornil(p_L, 10) || !lua_isnoneornil(p_L, 11)) {
		emit_flags |= godot::RenderingServer::PARTICLES_EMIT_FLAG_COLOR;
	}

	// custom：四分量任一提供则整体生效，缺失分量补默认值 0。
	// 注意：默认 ParticleProcessMaterial 下语义为 (rotation, age, animation, lifetime)。
	const float custom_x = lua_isnoneornil(p_L, 12) ? 0.0f : (float)luaL_checknumber(p_L, 12);
	const float custom_y = lua_isnoneornil(p_L, 13) ? 0.0f : (float)luaL_checknumber(p_L, 13);
	const float custom_z = lua_isnoneornil(p_L, 14) ? 0.0f : (float)luaL_checknumber(p_L, 14);
	const float custom_w = lua_isnoneornil(p_L, 15) ? 0.0f : (float)luaL_checknumber(p_L, 15);
	const godot::Color custom(custom_x, custom_y, custom_z, custom_w);
	if (!lua_isnoneornil(p_L, 12) || !lua_isnoneornil(p_L, 13) || !lua_isnoneornil(p_L, 14) || !lua_isnoneornil(p_L, 15)) {
		emit_flags |= godot::RenderingServer::PARTICLES_EMIT_FLAG_CUSTOM;
	}

	// 旋转/缩放：发光标志共用 EMIT_FLAG_ROTATION_SCALE，任一组提供即整体生效；
	// 旋转单位为度数，缩放缺失分量补默认值 1。
	const bool has_rotation = !lua_isnoneornil(p_L, 16) || !lua_isnoneornil(p_L, 17) || !lua_isnoneornil(p_L, 18);
	const bool has_scale = !lua_isnoneornil(p_L, 19) || !lua_isnoneornil(p_L, 20) || !lua_isnoneornil(p_L, 21);
	godot::Transform3D xform;
	xform.origin = position;
	if (has_rotation || has_scale) {
		const float rot_x = lua_isnoneornil(p_L, 16) ? 0.0f : (float)luaL_checknumber(p_L, 16);
		const float rot_y = lua_isnoneornil(p_L, 17) ? 0.0f : (float)luaL_checknumber(p_L, 17);
		const float rot_z = lua_isnoneornil(p_L, 18) ? 0.0f : (float)luaL_checknumber(p_L, 18);
		const float scale_x = lua_isnoneornil(p_L, 19) ? 1.0f : (float)luaL_checknumber(p_L, 19);
		const float scale_y = lua_isnoneornil(p_L, 20) ? 1.0f : (float)luaL_checknumber(p_L, 20);
		const float scale_z = lua_isnoneornil(p_L, 21) ? 1.0f : (float)luaL_checknumber(p_L, 21);
		const godot::Vector3 rotation(
			godot::Math::deg_to_rad(rot_x),
			godot::Math::deg_to_rad(rot_y),
			godot::Math::deg_to_rad(rot_z)
		);
		const godot::Vector3 scale(scale_x, scale_y, scale_z);
		xform.basis = godot::Basis::from_euler(rotation) * godot::Basis::from_scale(scale);
		emit_flags |= godot::RenderingServer::PARTICLES_EMIT_FLAG_ROTATION_SCALE;
	}

	particles->emit_particle(xform, velocity, color, custom, emit_flags);
	_push_bool(p_L, true);
	return 1;
}

static const luaL_Reg particles_funcs[] = {
	{"play", l_play},
	{"stop", l_stop},
	{"clear", l_clear},
	{"emit_particle", l_emit_particle},
	{"set_speed_scale", l_set_speed_scale},
	{"is_playing", l_is_playing},
	{"is_alive", l_is_alive},
	{nullptr, nullptr}
};

int luaopen_native_particles(lua_State *p_L) {
	luaL_newlib(p_L, particles_funcs);
	return 1;
}

} // namespace luagd
