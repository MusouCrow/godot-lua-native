#ifndef LUAGD_AI_MODULE_H
#define LUAGD_AI_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_ai 模块。
// 提供 NavigationAgent3D 的创建/销毁、路径查询、避障控制等原子 API。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_ai(lua_State *p_L);

// 清理 AI 模块资源。
// GDExtension 反初始化阶段只清理模块记录，场景对象交给引擎统一销毁。
// 约束：只允许在主线程调用。
// 在 LuaRuntime::shutdown 之前调用。
void ai_cleanup();

} // namespace luagd

#endif // LUAGD_AI_MODULE_H
