#ifndef LUAGD_GD_DISPLAY_H
#define LUAGD_GD_DISPLAY_H

struct lua_State;

namespace luagd {

// Open the gd.display module.
// Registers window_get_size, window_set_size functions.
// Returns 1 (the module table) on the Lua stack.
int luaopen_gd_display(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_GD_DISPLAY_H
