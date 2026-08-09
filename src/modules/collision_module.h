#ifndef LUAGD_COLLISION_MODULE_H
#define LUAGD_COLLISION_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_collision 模块。
// 提供碰撞检测与空间查询 API。
int luaopen_native_collision(lua_State *p_L);

// 清理碰撞模块资源。
// 在 LuaRuntime::shutdown 阶段调用，释放缓存的 Shape3D 资源。
void collision_cleanup();

// 注册碰撞模块的信号接收器类型。
// 在 GDExtension 初始化阶段调用，用于接收 Area3D 的触发信号。
void collision_register_signal_receivers();

} // namespace luagd

#endif // LUAGD_COLLISION_MODULE_H
