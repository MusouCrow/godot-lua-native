#ifndef LUAGD_SYSTEM_MODULE_H
#define LUAGD_SYSTEM_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_system 模块。
// 注册 get_name 函数。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_system(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_SYSTEM_MODULE_H
