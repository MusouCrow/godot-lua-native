#include "input_module.h"

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_joypad_button.hpp>
#include <godot_cpp/classes/input_event_joypad_motion.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// Lua registry 键名，用于存储回调函数引用
static const char *INPUT_CALLBACK_KEY = "native_input.input_callback";

// native_input.bind_input(func) -> void
// 绑定输入回调函数。
// func: 接收 (action, pressed, strength, device_type) 参数的函数。
static int l_bind_input(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_input.bind_input: expected 1 argument (function), got ", argc);
		return 0;
	}

	if (!lua_isfunction(p_L, 1)) {
		godot::UtilityFunctions::printerr("native_input.bind_input: argument must be a function");
		return 0;
	}

	// 将函数存储到 registry
	lua_pushvalue(p_L, 1);
	lua_setfield(p_L, LUA_REGISTRYINDEX, INPUT_CALLBACK_KEY);

	return 0;
}

// native_input.is_pressed(action) -> bool
// 返回 action 是否刚刚按下（按下瞬间）。
static int l_is_pressed(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_input.is_pressed: expected 1 argument (action), got ", argc);
		lua_pushboolean(p_L, false);
		return 1;
	}

	const char *action = luaL_checkstring(p_L, 1);
	godot::Input *input = godot::Input::get_singleton();
	bool result = input->is_action_just_pressed(action);
	lua_pushboolean(p_L, result);
	return 1;
}

// native_input.is_hold(action) -> bool
// 返回 action 是否持续按住。
static int l_is_hold(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_input.is_hold: expected 1 argument (action), got ", argc);
		lua_pushboolean(p_L, false);
		return 1;
	}

	const char *action = luaL_checkstring(p_L, 1);
	godot::Input *input = godot::Input::get_singleton();
	bool result = input->is_action_pressed(action);
	lua_pushboolean(p_L, result);
	return 1;
}

// native_input.is_released(action) -> bool
// 返回 action 是否刚刚弹起（弹起瞬间）。
static int l_is_released(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_input.is_released: expected 1 argument (action), got ", argc);
		lua_pushboolean(p_L, false);
		return 1;
	}

	const char *action = luaL_checkstring(p_L, 1);
	godot::Input *input = godot::Input::get_singleton();
	bool result = input->is_action_just_released(action);
	lua_pushboolean(p_L, result);
	return 1;
}

// native_input.get_strength(action) -> float
// 返回 action 的力度 (0.0~1.0)。
static int l_get_strength(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_input.get_strength: expected 1 argument (action), got ", argc);
		lua_pushnumber(p_L, 0.0);
		return 1;
	}

	const char *action = luaL_checkstring(p_L, 1);
	godot::Input *input = godot::Input::get_singleton();
	double result = input->get_action_strength(action);
	lua_pushnumber(p_L, result);
	return 1;
}

// native_input.get_axis(neg_action, pos_action) -> float
// 返回轴值 (-1.0~1.0)。
static int l_get_axis(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_input.get_axis: expected 2 arguments (neg_action, pos_action), got ", argc);
		lua_pushnumber(p_L, 0.0);
		return 1;
	}

	const char *neg_action = luaL_checkstring(p_L, 1);
	const char *pos_action = luaL_checkstring(p_L, 2);
	godot::Input *input = godot::Input::get_singleton();
	double result = input->get_axis(neg_action, pos_action);
	lua_pushnumber(p_L, result);
	return 1;
}

// native_input.get_vector(left, right, up, down) -> x, y
// 返回归一化 2D 向量。
static int l_get_vector(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 4) {
		godot::UtilityFunctions::printerr("native_input.get_vector: expected 4 arguments (left, right, up, down), got ", argc);
		lua_pushnumber(p_L, 0.0);
		lua_pushnumber(p_L, 0.0);
		return 2;
	}

	const char *left = luaL_checkstring(p_L, 1);
	const char *right = luaL_checkstring(p_L, 2);
	const char *up = luaL_checkstring(p_L, 3);
	const char *down = luaL_checkstring(p_L, 4);
	godot::Input *input = godot::Input::get_singleton();
	godot::Vector2 result = input->get_vector(left, right, up, down);
	lua_pushnumber(p_L, result.x);
	lua_pushnumber(p_L, result.y);
	return 2;
}

// native_input.vibrate(weak, strong, duration) -> void
// 触发手柄震动。
// weak: 弱马达强度 (0.0~1.0)
// strong: 强马达强度 (0.0~1.0)
// duration: 持续秒数
static int l_vibrate(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 3) {
		godot::UtilityFunctions::printerr("native_input.vibrate: expected 3 arguments (weak, strong, duration), got ", argc);
		return 0;
	}

	double weak = luaL_checknumber(p_L, 1);
	double strong = luaL_checknumber(p_L, 2);
	double duration = luaL_checknumber(p_L, 3);

	godot::Input *input = godot::Input::get_singleton();
	if (input != nullptr) {
		input->start_joy_vibration(0, weak, strong, duration);
	}

	return 0;
}

static const luaL_Reg input_funcs[] = {
	{"bind_input", l_bind_input},
	{"is_pressed", l_is_pressed},
	{"is_hold", l_is_hold},
	{"is_released", l_is_released},
	{"get_strength", l_get_strength},
	{"get_axis", l_get_axis},
	{"get_vector", l_get_vector},
	{"vibrate", l_vibrate},
	{nullptr, nullptr}
};

int luaopen_native_input(lua_State *p_L) {
	luaL_newlib(p_L, input_funcs);
	return 1;
}

// 获取设备类型字符串
static const char *get_device_type(const godot::InputEvent *p_event) {
	if (godot::Object::cast_to<godot::InputEventKey>(p_event) != nullptr) {
		return "keyboard";
	}
	if (godot::Object::cast_to<godot::InputEventJoypadButton>(p_event) != nullptr) {
		return "joypad";
	}
	if (godot::Object::cast_to<godot::InputEventJoypadMotion>(p_event) != nullptr) {
		return "joypad";
	}
	return "unknown";
}

void input_dispatch_event(lua_State *p_L, const godot::InputEvent *p_event) {
	if (p_L == nullptr || p_event == nullptr) {
		return;
	}

	// 从 registry 获取回调函数
	lua_getfield(p_L, LUA_REGISTRYINDEX, INPUT_CALLBACK_KEY);
	if (lua_isnil(p_L, -1)) {
		// 未绑定回调，静默跳过
		lua_pop(p_L, 1);
		return;
	}

	if (!lua_isfunction(p_L, -1)) {
		godot::UtilityFunctions::printerr("native_input: input callback is not a function");
		lua_pop(p_L, 1);
		return;
	}

	// 获取 InputMap 单例
	godot::InputMap *input_map = godot::InputMap::get_singleton();
	if (input_map == nullptr) {
		lua_pop(p_L, 1);
		return;
	}

	// 遍历所有已注册的 Action
	godot::TypedArray<godot::StringName> actions = input_map->get_actions();
	const char *device_type = get_device_type(p_event);

	for (int i = 0; i < actions.size(); ++i) {
		godot::StringName action_name = actions[i];

		// 跳过内置 UI action（以 "ui_" 开头）
		godot::String action_str = godot::String(action_name);
		if (action_str.begins_with("ui_")) {
			continue;
		}

		// 检查事件是否匹配此 action
		if (!p_event->is_action(action_name)) {
			continue;
		}

		// 获取事件信息
		bool pressed = p_event->is_pressed();
		double strength = p_event->get_action_strength(action_name);

		// 复制函数到栈顶（因为 pcall 会消耗它）
		lua_pushvalue(p_L, -1);

		// 压入参数
		godot::CharString utf8_action = action_str.utf8();
		lua_pushstring(p_L, utf8_action.get_data());
		lua_pushboolean(p_L, pressed);
		lua_pushnumber(p_L, strength);
		lua_pushstring(p_L, device_type);

		// 调用函数（4 个参数，0 个返回值）
		int call_result = lua_pcall(p_L, 4, 0, 0);
		if (call_result != LUA_OK) {
			const char *err = lua_tostring(p_L, -1);
			godot::String err_msg = "native_input: input callback error: ";
			err_msg += err ? err : "(unknown)";
			godot::UtilityFunctions::printerr(err_msg);
			lua_pop(p_L, 1);
		}
	}

	// 弹出回调函数
	lua_pop(p_L, 1);
}

} // namespace luagd
