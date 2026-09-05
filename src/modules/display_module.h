#ifndef LUAGD_DISPLAY_MODULE_H
#define LUAGD_DISPLAY_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_display 模块。
// 注册 window_get_size、window_set_size、window_get_mode、window_set_mode、content_scale_set_scale 函数。
// 并导出窗口模式枚举常量（WINDOW_MODE_*）。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_display(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_DISPLAY_MODULE_H
