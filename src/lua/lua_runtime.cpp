#include "lua_runtime.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../modules/gd_display.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace luagd {

lua_State *LuaRuntime::state = nullptr;

bool LuaRuntime::initialize() {
	if (state != nullptr) {
		return true;
	}

	state = luaL_newstate();
	if (state == nullptr) {
		godot::UtilityFunctions::printerr("LuaRuntime: failed to create Lua state");
		return false;
	}

	luaL_openlibs(state);

	// Register native.display module
	luaL_requiref(state, "native.display", luaopen_gd_display, 0);
	lua_pop(state, 1);

	return true;
}

void LuaRuntime::shutdown() {
	if (state != nullptr) {
		lua_close(state);
		state = nullptr;
	}
}

bool LuaRuntime::is_initialized() {
	return state != nullptr;
}

lua_State *LuaRuntime::get_state() {
	return state;
}

int LuaRuntime::run_file(const godot::String &p_path) {
	if (state == nullptr) {
		godot::UtilityFunctions::printerr("LuaRuntime.run_file: runtime not initialized");
		return -1;
	}

	// Read file content using Godot's FileAccess (supports res:// paths)
	godot::Ref<godot::FileAccess> file = godot::FileAccess::open(p_path, godot::FileAccess::READ);
	if (!file.is_valid()) {
		godot::String err_msg = "LuaRuntime.run_file: cannot open file '";
		err_msg += p_path;
		err_msg += "'";
		godot::UtilityFunctions::printerr(err_msg);
		return -1;
	}

	godot::String content = file->get_as_text();
	file->close();

	// Convert to UTF-8 for Lua
	godot::CharString utf8_content = content.utf8();
	godot::CharString utf8_path = p_path.utf8();

	// Load and execute
	int load_result = luaL_loadbuffer(state, utf8_content.get_data(), utf8_content.length(), utf8_path.get_data());
	if (load_result != LUA_OK) {
		const char *err = lua_tostring(state, -1);
		godot::String err_msg = "LuaRuntime.run_file: load error: ";
		err_msg += err ? err : "(unknown)";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pop(state, 1);
		return load_result;
	}

	int call_result = lua_pcall(state, 0, 1, 0);
	if (call_result != LUA_OK) {
		const char *err = lua_tostring(state, -1);
		godot::String err_msg = "LuaRuntime.run_file: runtime error: ";
		err_msg += err ? err : "(unknown)";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pop(state, 1);
		return call_result;
	}

	// Get return value (exit code) if it's an integer
	int exit_code = 0;
	if (lua_isinteger(state, -1)) {
		exit_code = (int)lua_tointeger(state, -1);
	}
	lua_pop(state, 1);

	return exit_code;
}

int LuaRuntime::run_string(const godot::String &p_code, const godot::String &p_chunk_name) {
	if (state == nullptr) {
		godot::UtilityFunctions::printerr("LuaRuntime.run_string: runtime not initialized");
		return -1;
	}

	godot::CharString utf8_code = p_code.utf8();
	godot::CharString utf8_chunk_name = p_chunk_name.utf8();

	int load_result = luaL_loadbuffer(state, utf8_code.get_data(), utf8_code.length(), utf8_chunk_name.get_data());
	if (load_result != LUA_OK) {
		const char *err = lua_tostring(state, -1);
		godot::String err_msg = "LuaRuntime.run_string: load error: ";
		err_msg += err ? err : "(unknown)";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pop(state, 1);
		return load_result;
	}

	int call_result = lua_pcall(state, 0, 1, 0);
	if (call_result != LUA_OK) {
		const char *err = lua_tostring(state, -1);
		godot::String err_msg = "LuaRuntime.run_string: runtime error: ";
		err_msg += err ? err : "(unknown)";
		godot::UtilityFunctions::printerr(err_msg);
		lua_pop(state, 1);
		return call_result;
	}

	// Get return value (exit code) if it's an integer
	int exit_code = 0;
	if (lua_isinteger(state, -1)) {
		exit_code = (int)lua_tointeger(state, -1);
	}
	lua_pop(state, 1);

	return exit_code;
}

} // namespace luagd
