#ifndef LUAGD_MATERIAL_MODULE_H
#define LUAGD_MATERIAL_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_material 模块。
// 提供基础材质参数写入 API。
int luaopen_native_material(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_MATERIAL_MODULE_H
