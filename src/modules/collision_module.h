#ifndef LUAGD_COLLISION_MODULE_H
#define LUAGD_COLLISION_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_collision 模块。
// 提供碰撞检测与空间查询 API。
int luaopen_native_collision(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_COLLISION_MODULE_H
