#ifndef LUAGD_RES_MODULE_H
#define LUAGD_RES_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_res 模块。
// 注册 load、unload、is_loaded 函数。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_res(lua_State *p_L);

// 清理所有已加载的资源。
// 在 LuaRuntime::shutdown() 时调用。
void res_cleanup();

} // namespace luagd

#endif // LUAGD_RES_MODULE_H
