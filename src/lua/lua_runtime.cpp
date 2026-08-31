#include "lua_runtime.h"
#include "lua_signal_binding.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "../modules/display_module.h"
#include "../modules/core_module.h"
#include "../modules/input_module.h"
#include "../modules/system_module.h"
#include "../modules/audio_module.h"
#include "../modules/network_module.h"
#include "../modules/anim_module.h"
#include "../modules/particles_module.h"
#include "../modules/node_module.h"
#include "../modules/transform_module.h"
#include "../modules/physics_module.h"
#include "../modules/collision_module.h"
#include "../modules/camera_module.h"
#include "../modules/material_module.h"
#include "../modules/res_module.h"
#include "../modules/debug_draw_module.h"
#include "../modules/skeleton_module.h"
#include "../modules/ai_module.h"
#include "../modules/ui_module.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace luagd {

// 统一错误处理器：为 Lua 运行时错误追加调用堆栈。
// 与 Lua 官方 lua.c 的 msghandler 行为一致。
static int lua_runtime_error_handler(lua_State *p_L) {
	const char *message = lua_tostring(p_L, 1);

	if (message == nullptr) {
		if (luaL_callmeta(p_L, 1, "__tostring") &&
				lua_type(p_L, -1) == LUA_TSTRING) {
			return 1;
		}

		message = lua_pushfstring(
				p_L,
				"(error object is a %s value)",
				luaL_typename(p_L, 1));
	}

	luaL_traceback(p_L, p_L, message, 1);
	return 1;
}

lua_State *LuaRuntime::state = nullptr;
bool LuaRuntime::fatal_error = false;
godot::String *LuaRuntime::fatal_error_message = nullptr;

bool LuaRuntime::has_fatal_error() {
	return fatal_error;
}

godot::String LuaRuntime::get_fatal_error() {
	if (fatal_error_message != nullptr) {
		return *fatal_error_message;
	}
	return godot::String();
}

void LuaRuntime::report_fatal_error(
		const godot::String &p_context,
		const godot::String &p_message) {
	// 只锁存首条致命错误，后续连锁错误不覆盖
	if (fatal_error) {
		return;
	}

	fatal_error = true;

	godot::String err_msg = p_context;
	if (p_message.length() > 0) {
		err_msg += ": ";
		err_msg += p_message;
	}

	fatal_error_message = new godot::String(err_msg);
	godot::UtilityFunctions::printerr(err_msg);
}

bool LuaRuntime::initialize() {
	if (state != nullptr) {
		return true;
	}

	// 清空上一轮的致命错误状态
	fatal_error = false;
	if (fatal_error_message != nullptr) {
		delete fatal_error_message;
		fatal_error_message = nullptr;
	}

	state = luaL_newstate();
	if (state == nullptr) {
		godot::UtilityFunctions::printerr("LuaRuntime: failed to create Lua state");
		return false;
	}

	luaL_openlibs(state);

	// 注册 native_core 模块
	luaL_requiref(state, "native_core", luaopen_native_core, 0);
	lua_pop(state, 1);

	// 注册 native_display 模块
	luaL_requiref(state, "native_display", luaopen_native_display, 0);
	lua_pop(state, 1);

	// 注册 native_input 模块
	luaL_requiref(state, "native_input", luaopen_native_input, 0);
	lua_pop(state, 1);

	// 注册 native_system 模块
	luaL_requiref(state, "native_system", luaopen_native_system, 0);
	lua_pop(state, 1);

	// 注册 native_audio 模块
	luaL_requiref(state, "native_audio", luaopen_native_audio, 0);
	lua_pop(state, 1);

	// 注册 native_anim 模块
	luaL_requiref(state, "native_anim", luaopen_native_anim, 0);
	lua_pop(state, 1);

	// 注册 native_particles 模块
	luaL_requiref(state, "native_particles", luaopen_native_particles, 0);
	lua_pop(state, 1);

	// 注册 native_node 模块
	luaL_requiref(state, "native_node", luaopen_native_node, 0);
	lua_pop(state, 1);

	// 注册 native_transform 模块
	luaL_requiref(state, "native_transform", luaopen_native_transform, 0);
	lua_pop(state, 1);

	// 注册 native_physics 模块
	luaL_requiref(state, "native_physics", luaopen_native_physics, 0);
	lua_pop(state, 1);

	// 注册 native_collision 模块
	luaL_requiref(state, "native_collision", luaopen_native_collision, 0);
	lua_pop(state, 1);

	// 注册 native_camera 模块
	luaL_requiref(state, "native_camera", luaopen_native_camera, 0);
	lua_pop(state, 1);

	// 注册 native_material 模块
	luaL_requiref(state, "native_material", luaopen_native_material, 0);
	lua_pop(state, 1);

	// 注册 native_res 模块
	luaL_requiref(state, "native_res", luaopen_native_res, 0);
	lua_pop(state, 1);

	// 注册 native_debug_draw 模块
	luaL_requiref(state, "native_debug_draw", luaopen_native_debug_draw, 0);
	lua_pop(state, 1);

	// 注册 native_skeleton 模块
	luaL_requiref(state, "native_skeleton", luaopen_native_skeleton, 0);
	lua_pop(state, 1);

	// 注册 native_ai 模块
	luaL_requiref(state, "native_ai", luaopen_native_ai, 0);
	lua_pop(state, 1);

	// 注册 native_ui 模块
	luaL_requiref(state, "native_ui", luaopen_native_ui, 0);
	lua_pop(state, 1);

	// 注册 native_network 模块
	luaL_requiref(state, "native_network", luaopen_native_network, 0);
	lua_pop(state, 1);

	return true;
}

void LuaRuntime::shutdown() {
	if (state == nullptr) {
		return;
	}

	debug_draw_cleanup();
	audio_cleanup();
	collision_cleanup();
	res_cleanup();
	anim_cleanup();
	ai_cleanup();
	node_cleanup();
	network_cleanup();
	lua_signal_binding_cleanup(state);

	lua_close(state);
	state = nullptr;
}

bool LuaRuntime::is_initialized() {
	return state != nullptr;
}

lua_State *LuaRuntime::get_state() {
	return state;
}

int LuaRuntime::pcall(
		lua_State *p_L,
		int p_arg_count,
		int p_result_count,
		const godot::String &p_context) {
	if (p_L == nullptr) {
		report_fatal_error(p_context, "(lua state is null)");
		return LUA_ERRRUN;
	}

	// 被调用函数位于当前栈顶之下 p_arg_count 个位置
	const int error_handler_index = lua_gettop(p_L) - p_arg_count;

	// 将错误处理器压到函数与参数之前
	lua_pushcfunction(p_L, lua_runtime_error_handler);
	lua_insert(p_L, error_handler_index);

	const int call_result = lua_pcall(
			p_L,
			p_arg_count,
			p_result_count,
			error_handler_index);

	lua_remove(p_L, error_handler_index);

	if (call_result != LUA_OK) {
		const char *err = lua_tostring(p_L, -1);
		report_fatal_error(p_context, err ? err : "(unknown)");
	}

	return call_result;
}

int LuaRuntime::run_file(const godot::String &p_path) {
	if (state == nullptr) {
		godot::UtilityFunctions::printerr("LuaRuntime.run_file: runtime not initialized");
		return -1;
	}

	// 通过 Godot 的 FileAccess 读取文件内容（支持 res:// 路径）
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

	// 转换为 UTF-8 供 Lua 使用
	godot::CharString utf8_content = content.utf8();
	godot::CharString utf8_path = p_path.utf8();

	// 加载并执行
	int load_result = luaL_loadbuffer(state, utf8_content.get_data(), utf8_content.length(), utf8_path.get_data());
	if (load_result != LUA_OK) {
		const char *err = lua_tostring(state, -1);
		report_fatal_error(
				"LuaRuntime.run_file: load error",
				err ? err : "(unknown)");
		lua_pop(state, 1);
		return load_result;
	}

	int call_result = LuaRuntime::pcall(
			state,
			0,
			1,
			"LuaRuntime.run_file: runtime error");
	if (call_result != LUA_OK) {
		lua_pop(state, 1);
		return call_result;
	}

	// 如果返回值是整数，则作为退出码
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
		report_fatal_error(
				"LuaRuntime.run_string: load error",
				err ? err : "(unknown)");
		lua_pop(state, 1);
		return load_result;
	}

	int call_result = LuaRuntime::pcall(
			state,
			0,
			1,
			"LuaRuntime.run_string: runtime error");
	if (call_result != LUA_OK) {
		lua_pop(state, 1);
		return call_result;
	}

	// 如果返回值是整数，则作为退出码
	int exit_code = 0;
	if (lua_isinteger(state, -1)) {
		exit_code = (int)lua_tointeger(state, -1);
	}
	lua_pop(state, 1);

	return exit_code;
}

} // namespace luagd
