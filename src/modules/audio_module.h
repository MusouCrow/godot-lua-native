#ifndef LUAGD_AUDIO_MODULE_H
#define LUAGD_AUDIO_MODULE_H

struct lua_State;

namespace luagd {

// 打开 native_audio 模块。
// 提供音频播放器创建/销毁、播放控制、AudioBus 操作等原子 API。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_audio(lua_State *p_L);

// 清理音频模块资源。
// 销毁所有播放器和容器节点。
// 在 LuaRuntime::shutdown 之前调用。
void audio_cleanup();

} // namespace luagd

#endif // LUAGD_AUDIO_MODULE_H
