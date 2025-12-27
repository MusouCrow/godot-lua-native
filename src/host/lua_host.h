#ifndef LUAGD_LUA_HOST_H
#define LUAGD_LUA_HOST_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>

namespace luagd {

// LuaHost: Godot singleton class that provides Lua execution interface to GDScript.
// All methods must be called from the main thread.
class LuaHost : public godot::Object {
	GDCLASS(LuaHost, godot::Object);

public:
	LuaHost();
	~LuaHost();

	// Execute a Lua file. Returns exit code (0 = success).
	// p_path: Supports res://, user://, and absolute paths.
	// See LuaRuntime::run_file() for details.
	int run_file(const godot::String &p_path);

	// Execute a Lua string. Returns exit code (0 = success).
	// p_code: Lua source code
	int run_string(const godot::String &p_code);

	// Singleton access
	static LuaHost *get_singleton();

protected:
	static void _bind_methods();

private:
	static LuaHost *singleton;
};

} // namespace luagd

#endif // LUAGD_LUA_HOST_H
