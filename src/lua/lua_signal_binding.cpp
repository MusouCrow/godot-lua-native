#include "lua_signal_binding.h"

#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

struct LuaSignalBinding {
	int32_t binding_id;
	godot::ObjectID source_id;
	godot::StringName signal_name;
	godot::Callable callable;
	godot::Object *receiver;
	int callback_ref;
	godot::String debug_name;
};

static godot::HashMap<int32_t, LuaSignalBinding> bindings;
static int32_t next_binding_id = 1;

static void disconnect_binding(lua_State *p_L, int32_t p_binding_id) {
	if (!bindings.has(p_binding_id)) {
		return;
	}

	LuaSignalBinding binding = bindings[p_binding_id];

	// 断开 Godot 信号
	godot::Object *source = godot::ObjectDB::get_instance(binding.source_id);
	if (source != nullptr && source->is_connected(binding.signal_name, binding.callable)) {
		source->disconnect(binding.signal_name, binding.callable);
	}

	// 释放 Lua 回调引用
	if (p_L != nullptr && binding.callback_ref != LUA_NOREF) {
		luaL_unref(p_L, LUA_REGISTRYINDEX, binding.callback_ref);
	}

	// 删除 receiver
	if (binding.receiver != nullptr) {
		memdelete(binding.receiver);
	}

	bindings.erase(p_binding_id);
}

int lua_signal_binding_ref_callback(lua_State *p_L, int p_callback_index) {
	if (p_L == nullptr) {
		return LUA_NOREF;
	}

	if (!lua_isfunction(p_L, p_callback_index)) {
		godot::UtilityFunctions::printerr("lua_signal_binding.ref_callback: argument must be a function");
		return LUA_NOREF;
	}

	lua_pushvalue(p_L, p_callback_index);
	return luaL_ref(p_L, LUA_REGISTRYINDEX);
}

int32_t lua_signal_binding_create_with_ref(
		lua_State *p_L,
		godot::Object *p_source,
		const godot::StringName &p_signal_name,
		godot::Object *p_receiver,
		const godot::Callable &p_callable,
		int p_callback_ref,
		const godot::String &p_debug_name) {

	if (p_L == nullptr || p_source == nullptr || p_receiver == nullptr) {
		return -1;
	}

	if (p_callback_ref == LUA_NOREF || p_callback_ref == LUA_REFNIL) {
		godot::UtilityFunctions::printerr("lua_signal_binding.create_with_ref: invalid callback ref");
		return -1;
	}

	// 连接 Godot 信号
	const godot::Error connect_err = p_source->connect(p_signal_name, p_callable);
	if (connect_err != godot::OK) {
		godot::UtilityFunctions::printerr(
				"lua_signal_binding.create_with_ref: connect failed (", p_debug_name, "), error ", connect_err);
		if (p_L != nullptr) {
			luaL_unref(p_L, LUA_REGISTRYINDEX, p_callback_ref);
		}
		memdelete(p_receiver);
		return -1;
	}

	const int32_t binding_id = next_binding_id++;

	LuaSignalBinding rec;
	rec.binding_id = binding_id;
	rec.source_id = p_source->get_instance_id();
	rec.signal_name = p_signal_name;
	rec.callable = p_callable;
	rec.receiver = p_receiver;
	rec.callback_ref = p_callback_ref;
	rec.debug_name = p_debug_name;

	bindings[binding_id] = rec;

	return binding_id;
}

void lua_signal_binding_disconnect(lua_State *p_L, int32_t p_binding_id) {
	disconnect_binding(p_L, p_binding_id);
}

void lua_signal_binding_disconnect_by_source(lua_State *p_L, godot::ObjectID p_source_id) {
	godot::Vector<int32_t> ids;

	for (const auto &kv : bindings) {
		if (kv.value.source_id == p_source_id) {
			ids.push_back(kv.key);
		}
	}

	for (int i = 0; i < ids.size(); i++) {
		disconnect_binding(p_L, ids[i]);
	}
}

void lua_signal_binding_cleanup(lua_State *p_L) {
	godot::Vector<int32_t> ids;

	for (const auto &kv : bindings) {
		ids.push_back(kv.key);
	}

	for (int i = 0; i < ids.size(); i++) {
		disconnect_binding(p_L, ids[i]);
	}

	bindings.clear();
	next_binding_id = 1;
}

bool lua_signal_binding_push_callback(lua_State *p_L, int p_callback_ref) {
	if (p_L == nullptr || p_callback_ref == LUA_NOREF || p_callback_ref == LUA_REFNIL) {
		return false;
	}

	lua_rawgeti(p_L, LUA_REGISTRYINDEX, p_callback_ref);
	if (!lua_isfunction(p_L, -1)) {
		lua_pop(p_L, 1);
		godot::UtilityFunctions::printerr("lua_signal_binding.push_callback: callback is not a function");
		return false;
	}

	return true;
}

void lua_signal_binding_call_no_return(
		lua_State *p_L,
		int p_arg_count,
		const godot::String &p_debug_name) {

	if (p_L == nullptr) {
		return;
	}

	const int call_result = lua_pcall(p_L, p_arg_count, 0, 0);
	if (call_result != LUA_OK) {
		const char *err = lua_tostring(p_L, -1);
		godot::String err_msg = "lua_signal_binding.call (";
		err_msg += p_debug_name;
		err_msg += "): callback error: ";
		err_msg += err ? err : "(unknown)";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pop(p_L, 1);
	}
}

} // namespace luagd
