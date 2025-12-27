#include "thread_check.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace luagd {

bool is_main_thread() {
	return godot::OS::get_singleton()->get_thread_caller_id() == godot::OS::get_singleton()->get_main_thread_id();
}

bool ensure_main_thread(const godot::String &p_context) {
	if (!is_main_thread()) {
		godot::String err_msg = p_context;
		err_msg += ": must be called from main thread";
		godot::UtilityFunctions::printerr(err_msg);
		return false;
	}
	return true;
}

} // namespace luagd
