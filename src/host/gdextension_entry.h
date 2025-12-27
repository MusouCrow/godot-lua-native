#ifndef LUAGD_GDEXTENSION_ENTRY_H
#define LUAGD_GDEXTENSION_ENTRY_H

#include <godot_cpp/core/class_db.hpp>

void initialize_luagd_module(godot::ModuleInitializationLevel p_level);
void uninitialize_luagd_module(godot::ModuleInitializationLevel p_level);

#endif // LUAGD_GDEXTENSION_ENTRY_H
