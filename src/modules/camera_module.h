#ifndef LUAGD_CAMERA_MODULE_H
#define LUAGD_CAMERA_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_camera 模块。
// 提供 Camera3D 的基础相机 API。
int luaopen_native_camera(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_CAMERA_MODULE_H
