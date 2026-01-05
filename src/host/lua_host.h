#ifndef LUAGD_LUA_HOST_H
#define LUAGD_LUA_HOST_H

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/string.hpp>

namespace luagd {

// LuaHost：向 GDScript 提供 Lua 执行接口的 Godot 单例类。
// 约束：所有方法必须在主线程调用。
class LuaHost : public godot::Object {
	GDCLASS(LuaHost, godot::Object);

public:
	LuaHost();
	~LuaHost();

	// 执行 Lua 文件。
	// 返回：退出码（0 = 成功）。
	// p_path: 支持 res://、user:// 和绝对路径，详见 LuaRuntime::run_file()。
	int run_file(const godot::String &p_path);

	// 执行 Lua 字符串。
	// 返回：退出码（0 = 成功）。
	// p_code: Lua 源代码
	int run_string(const godot::String &p_code);

	// 调用 Lua 的 update 回调。
	// 返回：成功返回 0，失败返回非零值。
	// p_delta: 距上一物理帧的秒数。
	int tick(double p_delta);

	// 调用 Lua 的 shutdown 回调。
	// 错误只打印，不影响退出流程。
	void shutdown();

	// 单例访问
	static LuaHost *get_singleton();

protected:
	static void _bind_methods();

private:
	static LuaHost *singleton;
};

} // namespace luagd

#endif // LUAGD_LUA_HOST_H
