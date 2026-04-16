#ifndef LUAGD_DEBUG_DRAW_MODULE_H
#define LUAGD_DEBUG_DRAW_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_debug_draw 模块。
// 提供运行时 3D 调试绘制接口。
int luaopen_native_debug_draw(lua_State *p_L);

// 清理 native_debug_draw 模块状态。
// 仅释放模块记录，不主动销毁场景对象。
// 约束：只允许在主线程调用。
void debug_draw_cleanup();

} // namespace luagd

#endif // LUAGD_DEBUG_DRAW_MODULE_H
