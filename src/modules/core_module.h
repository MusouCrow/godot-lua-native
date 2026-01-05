#ifndef LUAGD_CORE_MODULE_H
#define LUAGD_CORE_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_core 模块。
// 注册 bind_update、bind_shutdown 函数。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_core(lua_State *p_L);

// 调用 Lua 的 update 回调。
// p_delta: 距上一物理帧的秒数。
// 返回：成功返回 0，失败返回非零值。
int core_call_update(lua_State *p_L, double p_delta);

// 调用 Lua 的 shutdown 回调。
// 错误只打印，不影响退出流程。
void core_call_shutdown(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_CORE_MODULE_H
