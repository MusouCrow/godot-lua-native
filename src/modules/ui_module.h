#ifndef LUAGD_UI_MODULE_H
#define LUAGD_UI_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_ui 模块。
// 提供基于 ObjectID 句柄的 UI 节点操作 API。
// 支持 CanvasItem、Control、Range 类型的节点。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_ui(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_UI_MODULE_H
