#include "lua_host.h"

#include <godot_cpp/core/class_db.hpp>

#include "host_thread_check.h"
#include "../lua/lua_runtime.h"
#include "../modules/core_module.h"
#include "../modules/input_module.h"

#include <godot_cpp/classes/input_event.hpp>

namespace luagd {

LuaHost *LuaHost::singleton = nullptr;

LuaHost::LuaHost() {
	singleton = this;
}

LuaHost::~LuaHost() {
	singleton = nullptr;
}

LuaHost *LuaHost::get_singleton() {
	return singleton;
}

void LuaHost::_bind_methods() {
	godot::ClassDB::bind_method(godot::D_METHOD("run_file", "path"), &LuaHost::run_file);
	godot::ClassDB::bind_method(godot::D_METHOD("run_string", "code"), &LuaHost::run_string);
	godot::ClassDB::bind_method(godot::D_METHOD("tick", "delta"), &LuaHost::tick);
	godot::ClassDB::bind_method(godot::D_METHOD("shutdown"), &LuaHost::shutdown);
	godot::ClassDB::bind_method(godot::D_METHOD("input", "event"), &LuaHost::input);
}

int LuaHost::run_file(const godot::String &p_path) {
	if (!ensure_main_thread("LuaHost.run_file")) {
		return -1;
	}
	return LuaRuntime::run_file(p_path);
}

int LuaHost::run_string(const godot::String &p_code) {
	if (!ensure_main_thread("LuaHost.run_string")) {
		return -1;
	}
	return LuaRuntime::run_string(p_code);
}

int LuaHost::tick(double p_delta) {
	if (!ensure_main_thread("LuaHost.tick")) {
		return -1;
	}
	lua_State *L = LuaRuntime::get_state();
	if (L == nullptr) {
		return -1;
	}
	return core_call_update(L, p_delta);
}

void LuaHost::shutdown() {
	if (!ensure_main_thread("LuaHost.shutdown")) {
		return;
	}
	lua_State *L = LuaRuntime::get_state();
	if (L == nullptr) {
		return;
	}
	core_call_shutdown(L);
}

void LuaHost::input(const godot::Ref<godot::InputEvent> &p_event) {
	if (!ensure_main_thread("LuaHost.input")) {
		return;
	}
	lua_State *L = LuaRuntime::get_state();
	if (L == nullptr || p_event.is_null()) {
		return;
	}
	input_dispatch_event(L, p_event.ptr());
}

} // namespace luagd
