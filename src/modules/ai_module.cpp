#include "ai_module.h"

#include "node_module.h"

#include "../host/host_thread_check.h"
#include "../lua/lua_runtime.h"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/navigation_agent3d.hpp>
#include <godot_cpp/classes/navigation_server3d.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

struct NavigationAgentRecord {
	int32_t id;
	godot::NavigationAgent3D *agent;
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

// create(parent_node_id, path_desired_distance, target_desired_distance) -> agent_id
// 创建 NavigationAgent3D 并挂载到父节点。
static int l_create(lua_State *p_L) {
	const godot::ObjectID parent_id = godot::ObjectID((uint64_t)luaL_checkinteger(p_L, 1));
	const double path_desired_distance = luaL_checknumber(p_L, 2);
	const double target_desired_distance = luaL_checknumber(p_L, 3);

	godot::Node3D *parent = node_resolve(parent_id);
	if (parent == nullptr) {
		godot::UtilityFunctions::printerr("native_ai.create: parent node not found or invalid");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	godot::NavigationAgent3D *agent = memnew(godot::NavigationAgent3D);
	agent->set_path_desired_distance(path_desired_distance);
	agent->set_target_desired_distance(target_desired_distance);
	parent->add_child(agent);

	const int32_t agent_id = next_id++;
	NavigationAgentRecord rec;
	rec.id = agent_id;
	rec.agent = agent;

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
	if (rec.agent != nullptr && rec.agent->is_inside_tree()) {
		rec.agent->queue_free();
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

// is_navigation_finished(agent_id) -> bool
// 判断导航是否完成。
// 返回：true 表示导航已完成（到达目标或最后路径点）。
// 注意：返回 true 时应停止调用 get_next_path_position，避免站立抖动。
static int l_is_navigation_finished(lua_State *p_L) {
	const int32_t agent_id = (int32_t)luaL_checkinteger(p_L, 1);

	NavigationAgentRecord *rec = get_agent(agent_id, "is_navigation_finished");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	const bool finished = rec->agent->is_navigation_finished();
	lua_pushboolean(p_L, finished);
	return 1;
}

// map_get_closest_point(agent_id, x, y, z) -> x, y, z
// 查询导航网格上离给定坐标最近的点。
// 通过 agent_id 获取其所在的 navigation map，然后查询最近点。
static int l_map_get_closest_point(lua_State *p_L) {
	const int32_t agent_id = (int32_t)luaL_checkinteger(p_L, 1);
	const double x = luaL_checknumber(p_L, 2);
	const double y = luaL_checknumber(p_L, 3);
	const double z = luaL_checknumber(p_L, 4);

	NavigationAgentRecord *rec = get_agent(agent_id, "map_get_closest_point");
	if (rec == nullptr) {
		lua_pushnumber(p_L, x);
		lua_pushnumber(p_L, y);
		lua_pushnumber(p_L, z);
		return 3;
	}

	const godot::RID map = rec->agent->get_navigation_map();
	godot::NavigationServer3D *nav_server = godot::NavigationServer3D::get_singleton();
	const godot::Vector3 to_point(x, y, z);
	const godot::Vector3 closest_point = nav_server->map_get_closest_point(map, to_point);

	lua_pushnumber(p_L, closest_point.x);
	lua_pushnumber(p_L, closest_point.y);
	lua_pushnumber(p_L, closest_point.z);
	return 3;
}

static const luaL_Reg ai_funcs[] = {
	{"create", l_create},
	{"destroy", l_destroy},
	{"set_target_position", l_set_target_position},
	{"get_next_path_position", l_get_next_path_position},
	{"is_navigation_finished", l_is_navigation_finished},
	{"map_get_closest_point", l_map_get_closest_point},
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
