#ifndef LUAGD_PACKED_LUA_ARCHIVE_H
#define LUAGD_PACKED_LUA_ARCHIVE_H

#include <cstdint>

#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace luagd {

// 索引中的一条记录：模块字节码在归档数据中的区间。
struct PackedLuaEntry {
	uint64_t data_offset;
	uint64_t data_size;

	PackedLuaEntry();
	PackedLuaEntry(uint64_t p_data_offset, uint64_t p_data_size);
};

// 打开并解析 pack_lua.py 生成的 lua.dat 归档。
// 只负责解析与查询，不参与 Lua 栈、require、chunk 执行。
class PackedLuaArchive {
public:
	PackedLuaArchive();
	~PackedLuaArchive();

	// 打开并解析 lua.dat。
	// p_expected_lua_version: 运行时要求的 LUA_VERSION_RELEASE_NUM。
	// r_error: 失败时写入错误文本，可为空。
	// 返回：成功 true，失败 false。
	bool open(
			const godot::String &p_path,
			uint32_t p_expected_lua_version,
			godot::String *r_error);

	// 释放当前归档数据。
	void close();

	// 返回：当前是否已经成功打开归档。
	bool is_open() const;

	// 按 Lua 模块名查找字节码。
	// r_data 指向内部 PackedByteArray，调用方不得在使用期间
	// 调用 close()、open() 或销毁本对象。
	// 返回：模块存在并返回有效数据时 true。
	bool get_module(
			const godot::String &p_module_name,
			const uint8_t **r_data,
			uint64_t *r_size) const;

private:
	godot::PackedByteArray archive_data;
	godot::HashMap<godot::String, PackedLuaEntry> entries;
	godot::String archive_path;
	bool opened;

	bool parse_archive(
			const godot::PackedByteArray &p_data,
			uint32_t p_expected_lua_version,
			godot::String *r_error);

	bool read_u32(
			const uint8_t *p_data,
			uint64_t p_total_size,
			uint64_t *r_offset,
			uint32_t *r_value) const;

	bool read_u64(
			const uint8_t *p_data,
			uint64_t p_total_size,
			uint64_t *r_offset,
			uint64_t *r_value) const;

	bool read_module_name(
			const uint8_t *p_data,
			uint64_t p_total_size,
			uint64_t *r_offset,
			godot::String *r_module_name) const;
};

} // namespace luagd

#endif // LUAGD_PACKED_LUA_ARCHIVE_H