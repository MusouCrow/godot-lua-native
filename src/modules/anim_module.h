#ifndef LUAGD_ANIM_MODULE_H
#define LUAGD_ANIM_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_anim 模块。
// 提供 Animator / Layer 的基础动画控制 API。
int luaopen_native_anim(lua_State *p_L);

// 清理 native_anim 模块状态。
// 释放模块创建的 Animator 运行时对象。
void anim_cleanup();

} // namespace luagd

#endif // LUAGD_ANIM_MODULE_H
