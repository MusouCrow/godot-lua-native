#ifndef LUAGD_LUA_RUNTIME_H
#define LUAGD_LUA_RUNTIME_H

#include <godot_cpp/variant/string.hpp>

struct lua_State;

namespace luagd {

// Lua 运行时：管理全局 lua_State、模块注册与脚本执行。
// 约束：所有公开函数必须在主线程调用。
class LuaRuntime {
public:
	// 初始化全局 lua_State。
	// 约束：必须在扩展初始化时调用一次。
	// 返回：成功返回 true，失败返回 false。
	static bool initialize();

	// 关闭并销毁全局 lua_State。
	// 约束：必须在扩展终止时调用。
	static void shutdown();

	// 返回：运行时是否已初始化。
	static bool is_initialized();

	// 执行 Lua 文件。
	// 返回：成功返回 0，失败返回非零值（错误信息通过 Godot 的 print_error 输出）。
	//
	// p_path 支持以下路径格式：
	//   - "res://..." : Godot 资源路径（相对于项目根目录）
	//   - "user://..." : Godot 用户数据路径
	//   - 绝对路径（如 "/path/to/file.lua" 或 "C:/path/to/file.lua"）
	// 注意：路径解析由 Godot 的 FileAccess 处理，导出项目可能受沙盒限制。
	static int run_file(const godot::String &p_path);

	// 执行 Lua 字符串。
	// 返回：成功返回 0，失败返回非零值。
	// p_code: Lua 源代码
	// p_chunk_name: 用于错误消息的名称（可选，默认为 "=string"）
	static int run_string(const godot::String &p_code, const godot::String &p_chunk_name = "=string");

	// 获取全局 lua_State。
	// 返回：未初始化时返回 nullptr。
	static lua_State *get_state();

private:
	LuaRuntime() = delete;
	~LuaRuntime() = delete;

	static lua_State *state;
};

} // namespace luagd

#endif // LUAGD_LUA_RUNTIME_H
