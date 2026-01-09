#include "system_module.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// native_system.get_name() -> string
// 返回：操作系统名称字符串（如 "Windows", "macOS", "Linux", "Android", "iOS" 等）。
// OS 不可用时返回 "Unknown"。
static int l_get_name(lua_State *p_L) {
	godot::OS *os = godot::OS::get_singleton();
	if (os == nullptr) {
		godot::UtilityFunctions::printerr("native_system.get_name: OS not available");
		lua_pushliteral(p_L, "Unknown");
		return 1;
	}

	godot::String name = os->get_name();
	godot::CharString utf8_name = name.utf8();
	lua_pushstring(p_L, utf8_name.get_data());
	return 1;
}

static const luaL_Reg system_funcs[] = {
	{"get_name", l_get_name},
	{nullptr, nullptr}
};

int luaopen_native_system(lua_State *p_L) {
	luaL_newlib(p_L, system_funcs);
	return 1;
}

} // namespace luagd
