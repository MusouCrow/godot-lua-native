#ifndef LUAGD_HOST_THREAD_CHECK_H
#define LUAGD_HOST_THREAD_CHECK_H

#include <godot_cpp/variant/string.hpp>

namespace luagd {

// 检查当前是否在主线程执行。
// 返回：在主线程返回 true，否则返回 false。
bool is_main_thread();

// 断言当前在主线程执行。
// 如果不在主线程，打印错误消息并返回 false。
// p_context: 用于错误消息的函数/模块名称（如 "LuaHost.run_file"）
bool ensure_main_thread(const godot::String &p_context);

} // namespace luagd

#endif // LUAGD_HOST_THREAD_CHECK_H
