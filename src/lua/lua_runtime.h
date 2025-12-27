#ifndef LUAGD_LUA_RUNTIME_H
#define LUAGD_LUA_RUNTIME_H

#include <godot_cpp/variant/string.hpp>

struct lua_State;

namespace luagd {

// Lua runtime: manages global lua_State, module registration, and script execution.
// All public functions must be called from the main thread.
class LuaRuntime {
public:
	// Initialize the global Lua state. Returns true on success.
	// Must be called once at extension initialization.
	static bool initialize();

	// Shutdown and destroy the global Lua state.
	// Must be called at extension termination.
	static void shutdown();

	// Returns true if the runtime is initialized.
	static bool is_initialized();

	// Execute a Lua file. Returns 0 on success, non-zero on error.
	// On error, the error message is printed via Godot's print_error.
	//
	// p_path: Supports the following path formats:
	//   - "res://..." : Godot resource path (relative to project root)
	//   - "user://..." : Godot user data path
	//   - Absolute paths (e.g., "/path/to/file.lua" or "C:/path/to/file.lua")
	// Note: Path resolution is handled by Godot's FileAccess. Sandbox restrictions
	// may apply in exported projects.
	static int run_file(const godot::String &p_path);

	// Execute a Lua string. Returns 0 on success, non-zero on error.
	// p_code: Lua source code
	// p_chunk_name: name for error messages (optional, defaults to "=string")
	static int run_string(const godot::String &p_code, const godot::String &p_chunk_name = "=string");

	// Get the global lua_State. Returns nullptr if not initialized.
	static lua_State *get_state();

private:
	LuaRuntime() = delete;
	~LuaRuntime() = delete;

	static lua_State *state;
};

} // namespace luagd

#endif // LUAGD_LUA_RUNTIME_H
