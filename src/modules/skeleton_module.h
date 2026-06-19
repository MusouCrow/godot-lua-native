#ifndef LUAGD_SKELETON_MODULE_H
#define LUAGD_SKELETON_MODULE_H

struct lua_State;

namespace luagd {

int luaopen_native_skeleton(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_SKELETON_MODULE_H
