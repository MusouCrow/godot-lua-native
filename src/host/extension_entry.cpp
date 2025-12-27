#include "extension_entry.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>

#include "lua_host.h"
#include "../lua/lua_runtime.h"

using namespace godot;

static luagd::LuaHost *lua_host_singleton = nullptr;

void initialize_luagd_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Initialize Lua runtime
	luagd::LuaRuntime::initialize();

	// Register LuaHost class
	GDREGISTER_CLASS(luagd::LuaHost);

	// Create and register singleton
	lua_host_singleton = memnew(luagd::LuaHost);
	Engine::get_singleton()->register_singleton("LuaHost", lua_host_singleton);
}

void uninitialize_luagd_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	// Unregister singleton
	Engine::get_singleton()->unregister_singleton("LuaHost");
	if (lua_host_singleton != nullptr) {
		memdelete(lua_host_singleton);
		lua_host_singleton = nullptr;
	}

	// Shutdown Lua runtime
	luagd::LuaRuntime::shutdown();
}

extern "C" {

GDExtensionBool GDE_EXPORT luagd_library_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization *r_initialization) {
	GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_luagd_module);
	init_obj.register_terminator(uninitialize_luagd_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}

} // extern "C"
