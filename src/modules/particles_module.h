#ifndef LUAGD_PARTICLES_MODULE_H
#define LUAGD_PARTICLES_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_particles 模块。
// 提供 GPUParticles3D 的基础播放控制 API。
int luaopen_native_particles(lua_State *p_L);

} // namespace luagd

#endif // LUAGD_PARTICLES_MODULE_H
