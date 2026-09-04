#ifndef LUAGD_CORE_MODULE_H
#define LUAGD_CORE_MODULE_H

#include <godot_cpp/variant/string.hpp>

struct lua_State;

namespace luagd {

// 打开 native_core 模块。
// 注册 bind_update、bind_shutdown、quit、set_time_scale、get_time_scale、
// get_root_path、string_hash、get_unique_id 函数。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_core(lua_State *p_L);

// 调用 Lua 的 update 回调。
// p_delta: 距上一物理帧的秒数。
// 约束：只允许在主线程调用。
// 返回：成功返回 0，失败返回非零值。
int core_call_update(lua_State *p_L, double p_delta);

// 调用 Lua 的 shutdown 回调。
// 约束：只允许在主线程调用。
// 错误只打印，不影响退出流程。
void core_call_shutdown(lua_State *p_L);

// 调用 Lua 的致命错误善后回调。
// p_message: 致命错误文本，传入 Lua 侧用于报错上传。
// 约束：只允许在主线程调用。
// 错误只打印，不影响后续 terminate 与蓝屏显示流程。
void core_call_fatal(lua_State *p_L, const godot::String &p_message);

// 清理 native_core 持有的 Lua archive 缓存。
// 不访问 Lua 栈，可在 lua_close() 前后调用。
void core_cleanup();

} // namespace luagd

#endif // LUAGD_CORE_MODULE_H
