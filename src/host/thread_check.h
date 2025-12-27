#ifndef LUAGD_THREAD_CHECK_H
#define LUAGD_THREAD_CHECK_H

#include <godot_cpp/variant/string.hpp>

namespace luagd {

// Check if current execution is on the main thread.
// Returns true if on main thread, false otherwise.
bool is_main_thread();

// Assert that current execution is on the main thread.
// If not on main thread, prints error message and returns false.
// p_context: function/module name for error message (e.g., "LuaHost.run_file")
bool ensure_main_thread(const godot::String &p_context);

} // namespace luagd

#endif // LUAGD_THREAD_CHECK_H
