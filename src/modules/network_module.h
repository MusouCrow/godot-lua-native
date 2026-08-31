#ifndef LUAGD_NETWORK_MODULE_H
#define LUAGD_NETWORK_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_network 模块。
// 提供 HTTP POST 请求的发起与完成状态查询等原子 API。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_network(lua_State *p_L);

// 清理网络模块资源。
// GDExtension 反初始化阶段只清理模块记录，场景对象交给引擎统一销毁。
// 约束：只允许在主线程调用。
// 在 LuaRuntime::shutdown 之前调用。
void network_cleanup();

// 注册网络模块的信号接收器（NetworkSignalReceiver）。
// GDExtension 初始化阶段调用，仅需一次。
void network_register_signal_receivers();

} // namespace luagd

#endif // LUAGD_NETWORK_MODULE_H