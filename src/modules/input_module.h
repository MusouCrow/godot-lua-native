#ifndef LUAGD_INPUT_MODULE_H
#define LUAGD_INPUT_MODULE_H

struct lua_State;

namespace godot { class InputEvent; }

namespace luagd {

// 打开 native_input 模块。
int luaopen_native_input(lua_State *p_L);

// 将输入事件传递给 Lua 回调。
// p_event: Godot InputEvent。
// 匹配所有已注册的 Action，对每个匹配调用 Lua 回调。
void input_dispatch_event(lua_State *p_L, const godot::InputEvent *p_event);

} // namespace luagd

#endif // LUAGD_INPUT_MODULE_H
