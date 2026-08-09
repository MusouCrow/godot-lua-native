#ifndef LUAGD_LUA_SIGNAL_BINDING_H
#define LUAGD_LUA_SIGNAL_BINDING_H

#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

struct lua_State;

namespace godot {
class Object;
}

namespace luagd {

// 保存 Lua 回调函数到 registry，返回引用 id。
// 返回：成功返回 ref id，失败返回 LUA_NOREF。
// p_callback_index: 栈上函数所在位置。
int lua_signal_binding_ref_callback(lua_State *p_L, int p_callback_index);

// 登记一个 Lua 回调与 Godot 信号的绑定。
// 成功时绑定记录接管 p_receiver 与 p_callback_ref 的所有权。
// 失败时负责释放 p_receiver 与 p_callback_ref，返回 -1。
// 返回：binding_id（>=1），失败返回 -1。
int32_t lua_signal_binding_create_with_ref(
		lua_State *p_L,
		godot::Object *p_source,
		const godot::StringName &p_signal_name,
		godot::Object *p_receiver,
		const godot::Callable &p_callable,
		int p_callback_ref,
		const godot::String &p_debug_name);

// 断开指定 binding，释放 Lua 回调与 receiver。
void lua_signal_binding_disconnect(lua_State *p_L, int32_t p_binding_id);

// 断开所有 source_id 匹配的绑定。
void lua_signal_binding_disconnect_by_source(lua_State *p_L, godot::ObjectID p_source_id);

// 清理全部绑定，重置 id 分配。必须在 lua_close 前调用。
void lua_signal_binding_cleanup(lua_State *p_L);

// 将回调函数压入 Lua 栈顶。
// 返回：成功返回 true，栈顶为函数；失败返回 false。
bool lua_signal_binding_push_callback(lua_State *p_L, int p_callback_ref);

// 调用栈顶的 Lua 回调。约定：调用前栈上为 function + p_arg_count 个参数。
// 错误只打印，不影响 Godot signal 流程。
void lua_signal_binding_call_no_return(
		lua_State *p_L,
		int p_arg_count,
		const godot::String &p_debug_name);

} // namespace luagd

#endif // LUAGD_LUA_SIGNAL_BINDING_H
