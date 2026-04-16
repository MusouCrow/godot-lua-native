#ifndef LUAGD_PHYSICS_MODULE_H
#define LUAGD_PHYSICS_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_physics 模块。
// 提供基础物理与碰撞查询 API。
int luaopen_native_physics(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_PHYSICS_MODULE_H
