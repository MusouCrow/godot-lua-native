#ifndef LUAGD_FILE_MODULE_H
#define LUAGD_FILE_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_file Lua 模块。
// 返回：在 Lua 栈上压入 1 个模块表。
int luaopen_native_file(lua_State *p_L);

// 清理模块持有的所有 FileAccess 句柄。
// 约束：只能在主线程调用；不访问 Lua 栈。
void file_cleanup();

} // namespace luagd

#endif // LUAGD_FILE_MODULE_H