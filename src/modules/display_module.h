#ifndef LUAGD_DISPLAY_MODULE_H
#define LUAGD_DISPLAY_MODULE_H

struct lua_State;

namespace luagd {

// Open the native.display module.
// Registers window_get_size, window_set_size functions.
// Returns 1 (the module table) on the Lua stack.
int luaopen_native_display(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_DISPLAY_MODULE_H
