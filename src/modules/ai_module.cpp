#include "ai_module.h"

#include "node_module.h"

#include "../host/host_thread_check.h"
#include "../lua/lua_runtime.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/navigation_agent3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

struct NavigationAgentRecord {
	int32_t id;
	godot::NavigationAgent3D *agent;
	godot::Vector3 safe_velocity;
	godot::Callable velocity_computed_callback;
};

static godot::HashMap<int32_t, NavigationAgentRecord> agents;
static int32_t next_id = 1;

static NavigationAgentRecord *get_agent(int32_t p_id, const char *p_func_name) {
	if (!agents.has(p_id)) {
		godot::UtilityFunctions::printerr("native_ai.", p_func_name, ": invalid id ", p_id);
		return nullptr;
	}

	NavigationAgentRecord *rec = &agents[p_id];
	if (rec->agent == nullptr || !rec->agent->is_inside_tree()) {
		agents.erase(p_id);
		godot::UtilityFunctions::printerr("native_ai.", p_func_name, ": agent is no longer valid, id ", p_id);
		return nullptr;
	}

	return rec;
}

static void _on_velocity_computed_callback(godot::Vector3 p_safe_velocity, int32_t p_agent_id) {
	if (!agents.has(p_agent_id)) {
		return;
	}

	agents[p_agent_id].safe_velocity = p_safe_velocity;
}

// create(parent_node_id) -> agent_id
// 创建 NavigationAgent3D 并挂载到父节点。
static int l_create(lua_State *p_L) {
	const godot::ObjectID parent_id = godot::ObjectID((uint64_t)luaL_checkinteger(p_L, 1));

	godot::Node3D *parent = node_resolve(parent_id);
	if (parent == nullptr) {
		godot::UtilityFunctions::printerr("native_ai.create: parent node not found or invalid");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::NavigationAgent3D *agent = memnew(godot::NavigationAgent3D);
	agent->set_avoidance_enabled(true);
	parent->add_child(agent);

	const int32_t agent_id = next_id++;
	NavigationAgentRecord rec;
	rec.id = agent_id;
	rec.agent = agent;
	rec.safe_velocity = godot::Vector3(0, 0, 0);

	rec.velocity_computed_callback = callable_mp_static(&_on_velocity_computed_callback).bind(agent_id);
	agent->connect("velocity_computed", rec.velocity_computed_callback);

	agents[agent_id] = rec;

	lua_pushinteger(p_L, agent_id);
	return 1;
}

// destroy(agent_id) -> void
// 销毁 NavigationAgent3D。
static int l_destroy(lua_State *p_L) {
	const int32_t agent_id = (int32_t)luaL_checkinteger(p_L, 1);

	if (!agents.has(agent_id)) {
		return 0;
	}

	NavigationAgentRecord rec = agents[agent_id];
	if (rec.agent != nullptr) {
		if (rec.agent->is_connected("velocity_computed", rec.velocity_computed_callback)) {
			rec.agent->disconnect("velocity_computed", rec.velocity_computed_callback);
		}

		if (rec.agent->is_inside_tree()) {
			rec.agent->queue_free();
		}
	}

	agents.erase(agent_id);
	return 0;
}

// set_target_position(agent_id, x, y, z) -> void
// 设置目标位置。
static int l_set_target_position(lua_State *p_L) {
	const int32_t agent_id = (int32_t)luaL_checkinteger(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);

	NavigationAgentRecord *rec = get_agent(agent_id, "set_target_position");
	if (rec == nullptr) {
		return 0;
	}

	rec->agent->set_target_position(godot::Vector3(x, y, z));
	return 0;
}

// get_next_path_position(agent_id) -> x, y, z
// 获取下一个路径位置（每物理帧必须调用）。
static int l_get_next_path_position(lua_State *p_L) {
	const int32_t agent_id = (int32_t)luaL_checkinteger(p_L, 1);

	NavigationAgentRecord *rec = get_agent(agent_id, "get_next_path_position");
	if (rec == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	const godot::Vector3 pos = rec->agent->get_next_path_position();
	lua_pushnumber(p_L, pos.x);
	lua_pushnumber(p_L, pos.y);
	lua_pushnumber(p_L, pos.z);
	return 3;
}

// set_velocity(agent_id, vx, vy, vz) -> void
// 设置速度（用于避障计算）。
static int l_set_velocity(lua_State *p_L) {
	const int32_t agent_id = (int32_t)luaL_checkinteger(p_L, 1);
	const double vx = luaL_checknumber(p_L, 2);
	const double vy = luaL_checknumber(p_L, 3);
	const double vz = luaL_checknumber(p_L, 4);

	NavigationAgentRecord *rec = get_agent(agent_id, "set_velocity");
	if (rec == nullptr) {
		return 0;
	}

	rec->agent->set_velocity(godot::Vector3(vx, vy, vz));
	return 0;
}

// get_safe_velocity(agent_id) -> vx, vy, vz
// 获取安全速度（由 velocity_computed 信号更新）。
static int l_get_safe_velocity(lua_State *p_L) {
	const int32_t agent_id = (int32_t)luaL_checkinteger(p_L, 1);

	NavigationAgentRecord *rec = get_agent(agent_id, "get_safe_velocity");
	if (rec == nullptr) {
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		lua_pushnumber(p_L, 0);
		return 3;
	}

	lua_pushnumber(p_L, rec->safe_velocity.x);
	lua_pushnumber(p_L, rec->safe_velocity.y);
	lua_pushnumber(p_L, rec->safe_velocity.z);
	return 3;
}

static const luaL_Reg ai_funcs[] = {
	{"create", l_create},
	{"destroy", l_destroy},
	{"set_target_position", l_set_target_position},
	{"get_next_path_position", l_get_next_path_position},
	{"set_velocity", l_set_velocity},
	{"get_safe_velocity", l_get_safe_velocity},
	{nullptr, nullptr}
};

int luaopen_native_ai(lua_State *p_L) {
	luaL_newlib(p_L, ai_funcs);
	return 1;
}

void ai_cleanup() {
	agents.clear();
	next_id = 1;
}

} // namespace luagd
