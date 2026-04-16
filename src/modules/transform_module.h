#ifndef LUAGD_TRANSFORM_MODULE_H
#define LUAGD_TRANSFORM_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_transform 模块。
// 提供 Node3D 的基础变换 API。
int luaopen_native_transform(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_TRANSFORM_MODULE_H
