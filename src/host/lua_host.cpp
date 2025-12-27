#include "lua_host.h"

#include <godot_cpp/core/class_db.hpp>

#include "host_thread_check.h"
#include "../lua/lua_runtime.h"

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

} // namespace luagd
