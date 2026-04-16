#include "particles_module.h"

#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

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

// update(node_id, delta) -> bool
// 请求粒子在单帧内额外处理一段时间。
static int l_update(lua_State *p_L) {
	const godot::ObjectID node_id = _read_node_id(p_L, 1);
	const double delta = luaL_checknumber(p_L, 2);
	if (delta < 0.0) {
		godot::UtilityFunctions::printerr("native_particles.update: delta must be >= 0, got ", delta);
		_push_bool(p_L, false);
		return 1;
	}

	godot::GPUParticles3D *particles = _resolve_particles(node_id, "update");
	if (particles == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	particles->request_particles_process((float)delta);
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

static const luaL_Reg particles_funcs[] = {
	{"play", l_play},
	{"stop", l_stop},
	{"clear", l_clear},
	{"update", l_update},
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
