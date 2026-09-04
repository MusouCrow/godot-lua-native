#include "file_module.h"

#include "../host/host_thread_check.h"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/templates/hash_map.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#include <cstring>

namespace luagd {

// 模块级静态状态。
// Lua 只持有模块自管的整数 ID，不持有 FileAccess 对象。
// 所有句柄必须显式调用 close；file_cleanup() 负责清理未关闭句柄。
static godot::HashMap<int64_t, godot::Ref<godot::FileAccess>> open_files;
static int64_t next_file_id = 1;

// 获取已打开的 FileAccess 引用。
// 句柄不存在时打印错误并返回 nullptr。
// 不转移所有权，不从 HashMap 中删除记录。
static godot::Ref<godot::FileAccess> *get_open_file(
		int64_t p_file_id,
		const char *p_func_name) {
	if (!open_files.has(p_file_id)) {
		godot::UtilityFunctions::printerr(
				"native_file.", p_func_name, ": invalid file handle ", p_file_id);
		return nullptr;
	}
	return &open_files[p_file_id];
}

// 将 Lua 字节串复制为 PackedByteArray。
// 必须保留完整字节长度（录像 payload 可能包含 \0）。
static godot::PackedByteArray make_packed_byte_array(
		const char *p_data,
		size_t p_length) {
	godot::PackedByteArray result;
	result.resize(static_cast<int64_t>(p_length));
	if (p_length > 0) {
		uint8_t *buffer = result.ptrw();
		memcpy(buffer, p_data, p_length);
	}
	return result;
}

// 将 PackedByteArray 按原始字节压入 Lua 字符串。
// 数组为空时压入空字符串，不转换为 Godot String。
static void push_packed_byte_array(
		lua_State *p_L,
		const godot::PackedByteArray &p_data) {
	int64_t size = p_data.size();
	if (size <= 0) {
		lua_pushlstring(p_L, "", 0);
		return;
	}
	const uint8_t *bytes = p_data.ptr();
	lua_pushlstring(
			p_L,
			reinterpret_cast<const char *>(bytes),
			static_cast<size_t>(size));
}

// 清理临时文件。仅用于内部清理，不对 Lua 暴露。
static bool remove_file(const godot::String &p_path) {
	godot::Error err = godot::DirAccess::remove_absolute(p_path);
	return err == godot::Error::OK;
}

// native_file.make_dir_recursive(path) -> boolean
// 递归创建目录。
static int l_make_dir_recursive(lua_State *p_L) {
	if (!ensure_main_thread("native_file.make_dir_recursive")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_file.make_dir_recursive: expected 1 argument (string)");
		lua_pushboolean(p_L, false);
		return 1;
	}

	const char *path = luaL_checkstring(p_L, 1);
	if (path == nullptr || path[0] == '\0') {
		godot::UtilityFunctions::printerr("native_file.make_dir_recursive: path must be non-empty");
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::String gd_path = path;
	godot::Error err = godot::DirAccess::make_dir_recursive_absolute(gd_path);
	if (err != godot::Error::OK) {
		godot::UtilityFunctions::printerr(
				"native_file.make_dir_recursive: failed, path=", gd_path,
				", error=", (int64_t)err);
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, true);
	return 1;
}

// native_file.write_file(path, data) -> boolean
// 覆盖写入文件，采用临时文件替换避免直接覆盖损坏。
static int l_write_file(lua_State *p_L) {
	if (!ensure_main_thread("native_file.write_file")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_file.write_file: expected 2 arguments (string, string)");
		lua_pushboolean(p_L, false);
		return 1;
	}

	const char *path = luaL_checkstring(p_L, 1);
	if (path == nullptr || path[0] == '\0') {
		godot::UtilityFunctions::printerr("native_file.write_file: path must be non-empty");
		lua_pushboolean(p_L, false);
		return 1;
	}

	size_t data_length = 0;
	const char *data = luaL_checklstring(p_L, 2, &data_length);

	godot::String gd_path = path;
	godot::String temp_path = gd_path;
	temp_path += ".tmp";

	godot::PackedByteArray buffer = make_packed_byte_array(data, data_length);

	godot::Ref<godot::FileAccess> file =
			godot::FileAccess::open(temp_path, godot::FileAccess::WRITE);
	if (!file.is_valid()) {
		godot::UtilityFunctions::printerr(
				"native_file.write_file: cannot open temp file, path=", temp_path);
		lua_pushboolean(p_L, false);
		return 1;
	}

	bool success = true;
	if (!file->store_buffer(buffer)) {
		success = false;
	}
	if (success) {
		file->flush();
	}
	file->close();

	if (success) {
		godot::Error rename_err = godot::DirAccess::rename_absolute(temp_path, gd_path);
		if (rename_err != godot::Error::OK) {
			remove_file(temp_path);
			godot::UtilityFunctions::printerr(
					"native_file.write_file: rename failed, path=", gd_path,
					", error=", (int64_t)rename_err);
			success = false;
		}
	} else {
		remove_file(temp_path);
	}

	lua_pushboolean(p_L, success);
	return 1;
}

// native_file.read_file(path) -> string|nil
// 读取完整文件；文件不存在或读取失败返回 nil。
static int l_read_file(lua_State *p_L) {
	if (!ensure_main_thread("native_file.read_file")) {
		lua_pushnil(p_L);
		return 1;
	}

	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_file.read_file: expected 1 argument (string)");
		lua_pushnil(p_L);
		return 1;
	}

	const char *path = luaL_checkstring(p_L, 1);
	if (path == nullptr || path[0] == '\0') {
		godot::UtilityFunctions::printerr("native_file.read_file: path must be non-empty");
		lua_pushnil(p_L);
		return 1;
	}

	godot::String gd_path = path;
	godot::Ref<godot::FileAccess> file =
			godot::FileAccess::open(gd_path, godot::FileAccess::READ);
	if (!file.is_valid()) {
		lua_pushnil(p_L);
		return 1;
	}

	uint64_t length = file->get_length();
	if (length == 0) {
		file->close();
		lua_pushlstring(p_L, "", 0);
		return 1;
	}

	godot::PackedByteArray buffer = file->get_buffer(static_cast<int64_t>(length));
	if (buffer.size() != static_cast<int64_t>(length)) {
		file->close();
		lua_pushnil(p_L);
		return 1;
	}

	file->close();
	push_packed_byte_array(p_L, buffer);
	return 1;
}

// native_file.append_file(path, data) -> boolean
// 追加写入录像 payload，不会覆盖已有内容。
static int l_append_file(lua_State *p_L) {
	if (!ensure_main_thread("native_file.append_file")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_file.append_file: expected 2 arguments (string, string)");
		lua_pushboolean(p_L, false);
		return 1;
	}

	const char *path = luaL_checkstring(p_L, 1);
	if (path == nullptr || path[0] == '\0') {
		godot::UtilityFunctions::printerr("native_file.append_file: path must be non-empty");
		lua_pushboolean(p_L, false);
		return 1;
	}

	size_t data_length = 0;
	const char *data = luaL_checklstring(p_L, 2, &data_length);

	godot::String gd_path = path;
	bool exists = godot::FileAccess::file_exists(gd_path);

	// FileAccess 没有独立的 APPEND 模式：
	// READ_WRITE 不会创建不存在的文件，WRITE 会清空已有文件，只能用于不存在的情况。
	godot::Ref<godot::FileAccess> file = godot::FileAccess::open(
			gd_path,
			exists ? godot::FileAccess::READ_WRITE : godot::FileAccess::WRITE);
	if (!file.is_valid()) {
		godot::UtilityFunctions::printerr(
				"native_file.append_file: cannot open file, path=", gd_path);
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (exists) {
		file->seek_end();
	}

	godot::PackedByteArray buffer = make_packed_byte_array(data, data_length);
	bool success = file->store_buffer(buffer);
	if (success) {
		file->flush();
	}
	file->close();

	lua_pushboolean(p_L, success);
	return 1;
}

// native_file.open_read(path) -> integer|nil
// 打开文件供流式读取，返回模块自管的整数句柄。
static int l_open_read(lua_State *p_L) {
	if (!ensure_main_thread("native_file.open_read")) {
		lua_pushnil(p_L);
		return 1;
	}

	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_file.open_read: expected 1 argument (string)");
		lua_pushnil(p_L);
		return 1;
	}

	const char *path = luaL_checkstring(p_L, 1);
	if (path == nullptr || path[0] == '\0') {
		godot::UtilityFunctions::printerr("native_file.open_read: path must be non-empty");
		lua_pushnil(p_L);
		return 1;
	}

	godot::String gd_path = path;
	godot::Ref<godot::FileAccess> file =
			godot::FileAccess::open(gd_path, godot::FileAccess::READ);
	if (!file.is_valid()) {
		godot::UtilityFunctions::printerr(
				"native_file.open_read: failed to open, path=", gd_path);
		lua_pushnil(p_L);
		return 1;
	}

	if (next_file_id <= 0) {
		godot::UtilityFunctions::printerr("native_file.open_read: file handle overflow");
		file->close();
		lua_pushnil(p_L);
		return 1;
	}

	int64_t file_id = next_file_id;
	next_file_id++;
	open_files[file_id] = file;

	lua_pushinteger(p_L, file_id);
	return 1;
}

// native_file.read(handle, length) -> string|nil
// 从打开的句柄读取最多 length 字节。
// EOF 或短读时返回实际读取到的字节，不视为错误。
static int l_read(lua_State *p_L) {
	if (!ensure_main_thread("native_file.read")) {
		lua_pushnil(p_L);
		return 1;
	}

	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_file.read: expected 2 arguments (integer, integer)");
		lua_pushnil(p_L);
		return 1;
	}

	int64_t handle = (int64_t)luaL_checkinteger(p_L, 1);
	int64_t length = (int64_t)luaL_checkinteger(p_L, 2);

	if (handle <= 0) {
		godot::UtilityFunctions::printerr("native_file.read: invalid file handle ", handle);
		lua_pushnil(p_L);
		return 1;
	}
	if (length < 0) {
		godot::UtilityFunctions::printerr("native_file.read: length must be >= 0");
		lua_pushnil(p_L);
		return 1;
	}

	godot::Ref<godot::FileAccess> *file = get_open_file(handle, "read");
	if (file == nullptr) {
		lua_pushnil(p_L);
		return 1;
	}

	godot::PackedByteArray buffer = (*file)->get_buffer(length);
	push_packed_byte_array(p_L, buffer);
	return 1;
}

// native_file.close(handle) -> boolean
// 关闭打开的句柄。重复关闭返回 false。
static int l_close(lua_State *p_L) {
	if (!ensure_main_thread("native_file.close")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	int argc = lua_gettop(p_L);
	if (argc < 1) {
		godot::UtilityFunctions::printerr("native_file.close: expected 1 argument (integer)");
		lua_pushboolean(p_L, false);
		return 1;
	}

	int64_t handle = (int64_t)luaL_checkinteger(p_L, 1);
	if (handle <= 0) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Ref<godot::FileAccess> *file = get_open_file(handle, "close");
	if (file == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	(*file)->close();
	open_files.erase(handle);

	lua_pushboolean(p_L, true);
	return 1;
}

static const luaL_Reg file_funcs[] = {
	{"make_dir_recursive", l_make_dir_recursive},
	{"write_file", l_write_file},
	{"read_file", l_read_file},
	{"append_file", l_append_file},
	{"open_read", l_open_read},
	{"read", l_read},
	{"close", l_close},
	{nullptr, nullptr}
};

int luaopen_native_file(lua_State *p_L) {
	luaL_newlib(p_L, file_funcs);
	return 1;
}

// 清理所有遗留句柄并重置句柄分配。
void file_cleanup() {
	if (!ensure_main_thread("native_file.file_cleanup")) {
		return;
	}

	for (godot::HashMap<int64_t, godot::Ref<godot::FileAccess>>::Iterator it =
			open_files.begin();
			it != open_files.end(); ++it) {
		if (it->value.is_valid()) {
			it->value->close();
		}
	}

	open_files.clear();
	next_file_id = 1;
}

} // namespace luagd