#include "system_module.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
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

// native_system.get_rendering_method() -> string
// 返回当前渲染方法；OS 不可用时返回 "Unknown"。
static int l_get_rendering_method(lua_State *p_L) {
	godot::OS *os = godot::OS::get_singleton();
	if (os == nullptr) {
		godot::UtilityFunctions::printerr("native_system.get_rendering_method: OS not available");
		lua_pushliteral(p_L, "Unknown");
		return 1;
	}

	const godot::String rendering_method = godot::RenderingServer::get_singleton()->get_current_rendering_method();
	const godot::CharString utf8_rendering_method = rendering_method.utf8();
	lua_pushstring(p_L, utf8_rendering_method.get_data());
	return 1;
}

// native_system.get_unique_id() -> string
// 获取设备唯一标识符。
// 注意：该字符串在重装系统、升级或修改硬件后可能变化，不可用于持久数据加密；也可能被外部程序伪造，不可用于安全校验。
// OS 不可用时返回空串。
static int l_get_unique_id(lua_State *p_L) {
	godot::OS *os = godot::OS::get_singleton();
	if (os == nullptr) {
		godot::UtilityFunctions::printerr("native_system.get_unique_id: OS not available");
		lua_pushliteral(p_L, "");
		return 1;
	}

	godot::String unique_id = os->get_unique_id();
	lua_pushstring(p_L, unique_id.utf8().get_data());
	return 1;
}

// native_system.get_locale() -> string
// 获取宿主操作系统的区域设置（locale），与 Godot 的 OS.get_locale() 一致。
// 返回：形如 language_Script_COUNTRY_VARIANT@extra 的字符串，language 之后的部分均为可选。
// 注意：如需仅获取 2 或 3 字母语言代码，请使用 OS.get_locale_language()。
// OS 不可用时返回空串。
static int l_get_locale(lua_State *p_L) {
	godot::OS *os = godot::OS::get_singleton();
	if (os == nullptr) {
		godot::UtilityFunctions::printerr("native_system.get_locale: OS not available");
		lua_pushliteral(p_L, "");
		return 1;
	}

	godot::String locale = os->get_locale();
	lua_pushstring(p_L, locale.utf8().get_data());
	return 1;
}

static const luaL_Reg system_funcs[] = {
	{"get_name", l_get_name},
	{"get_rendering_method", l_get_rendering_method},
	{"get_unique_id", l_get_unique_id},
	{"get_locale", l_get_locale},
	{nullptr, nullptr}
};

int luaopen_native_system(lua_State *p_L) {
	luaL_newlib(p_L, system_funcs);
	return 1;
}

} // namespace luagd