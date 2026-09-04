#include "packed_lua_archive.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace luagd {

// 与 tools/pack_lua.py 中的格式常量保持一致。
static const char ARCHIVE_MAGIC[8] = {
	'R', 'A', 'B', 'I', 'B', 'A', 'N', 'A'
};
static const uint32_t FORMAT_VERSION = 1;
static const uint32_t HEADER_SIZE = 40;
static const uint32_t MAX_MODULE_NAME_SIZE = 4096;
static const uint32_t MAX_ENTRY_COUNT = 1000000;

PackedLuaEntry::PackedLuaEntry() :
		data_offset(0),
		data_size(0) {
}

PackedLuaEntry::PackedLuaEntry(
		uint64_t p_data_offset,
		uint64_t p_data_size) :
		data_offset(p_data_offset),
		data_size(p_data_size) {
}

PackedLuaArchive::PackedLuaArchive() :
		opened(false) {
}

PackedLuaArchive::~PackedLuaArchive() {
	close();
}

bool PackedLuaArchive::open(
		const godot::String &p_path,
		uint32_t p_expected_lua_version,
		godot::String *r_error) {
	close();

	godot::Ref<godot::FileAccess> file =
			godot::FileAccess::open(p_path, godot::FileAccess::READ);
	if (file.is_null()) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: cannot read '" + p_path + "'";
		}
		return false;
	}

	const uint64_t file_size = file->get_length();
	if (file_size == 0) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: empty file '" + p_path + "'";
		}
		return false;
	}

	godot::PackedByteArray loaded_data =
			file->get_buffer(static_cast<int64_t>(file_size));
	file->close();

	if (!parse_archive(
			loaded_data,
			p_expected_lua_version,
			r_error)) {
		return false;
	}

	archive_data = loaded_data;
	archive_path = p_path;
	opened = true;
	return true;
}

void PackedLuaArchive::close() {
	archive_data = godot::PackedByteArray();
	entries.clear();
	archive_path = godot::String();
	opened = false;
}

bool PackedLuaArchive::is_open() const {
	return opened;
}

bool PackedLuaArchive::get_module(
		const godot::String &p_module_name,
		const uint8_t **r_data,
		uint64_t *r_size) const {
	if (!opened ||
			r_data == nullptr ||
			r_size == nullptr) {
		return false;
	}

	const PackedLuaEntry *entry =
			entries.getptr(p_module_name);

	if (entry == nullptr) {
		return false;
	}

	if (entry->data_offset > static_cast<uint64_t>(archive_data.size())) {
		return false;
	}

	*r_data = archive_data.ptr() + entry->data_offset;
	*r_size = entry->data_size;
	return true;
}

bool PackedLuaArchive::parse_archive(
		const godot::PackedByteArray &p_data,
		uint32_t p_expected_lua_version,
		godot::String *r_error) {
	if (p_data.is_empty()) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: empty archive";
		}
		return false;
	}

	const uint8_t *data = p_data.ptr();
	const uint64_t total_size = static_cast<uint64_t>(p_data.size());

	if (total_size < HEADER_SIZE) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: truncated header";
		}
		return false;
	}

	for (uint32_t i = 0; i < 8; i++) {
		if (data[i] != static_cast<uint8_t>(ARCHIVE_MAGIC[i])) {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: invalid magic";
			}
			return false;
		}
	}

	uint64_t offset = 8;
	uint32_t format_version = 0;
	uint32_t lua_version = 0;
	uint32_t flags = 0;
	uint32_t entry_count = 0;
	uint64_t index_offset = 0;
	uint64_t index_size = 0;

	if (!read_u32(data, total_size, &offset, &format_version) ||
			!read_u32(data, total_size, &offset, &lua_version) ||
			!read_u32(data, total_size, &offset, &flags) ||
			!read_u32(data, total_size, &offset, &entry_count) ||
			!read_u64(data, total_size, &offset, &index_offset) ||
			!read_u64(data, total_size, &offset, &index_size)) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: truncated header";
		}
		return false;
	}

	if (format_version != FORMAT_VERSION) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: unsupported format version";
		}
		return false;
	}

	if (lua_version != p_expected_lua_version) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: lua version mismatch";
		}
		return false;
	}

	if (flags != 0) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: unsupported flags";
		}
		return false;
	}

	if (entry_count > MAX_ENTRY_COUNT) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: invalid entry count";
		}
		return false;
	}

	if (index_offset != HEADER_SIZE) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: invalid index offset";
		}
		return false;
	}

	if (index_offset > total_size ||
			index_size > total_size - index_offset) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: index out of bounds";
		}
		return false;
	}

	const uint64_t data_begin = index_offset + index_size;
	uint64_t cursor = index_offset;

	godot::HashMap<godot::String, PackedLuaEntry> local_entries;

	for (uint32_t i = 0; i < entry_count; i++) {
		godot::String module_name;
		if (!read_module_name(
				data,
				total_size,
				&cursor,
				&module_name)) {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: invalid module name";
			}
			return false;
		}

		uint64_t data_offset = 0;
		uint64_t data_size = 0;
		if (!read_u64(data, total_size, &cursor, &data_offset) ||
				!read_u64(data, total_size, &cursor, &data_size)) {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: truncated index entry";
			}
			return false;
		}

		if (data_offset < data_begin) {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: bytecode overlaps index";
			}
			return false;
		}

		if (data_offset > total_size ||
				data_size > total_size - data_offset) {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: bytecode range out of bounds";
			}
			return false;
		}

		if (data_size == 0) {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: empty bytecode";
			}
			return false;
		}

		if (data_size < 4 ||
				data[data_offset] != 0x1b ||
				data[data_offset + 1] != 'L' ||
				data[data_offset + 2] != 'u' ||
				data[data_offset + 3] != 'a') {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: invalid Lua bytecode";
			}
			return false;
		}

		if (local_entries.has(module_name)) {
			if (r_error != nullptr) {
				*r_error = "PackedLuaArchive: duplicate module '" +
						module_name + "'";
			}
			return false;
		}

		local_entries.insert(
				module_name,
				PackedLuaEntry(data_offset, data_size));
	}

	if (cursor != data_begin) {
		if (r_error != nullptr) {
			*r_error = "PackedLuaArchive: invalid index size";
		}
		return false;
	}

	entries = local_entries;
	return true;
}

bool PackedLuaArchive::read_u32(
		const uint8_t *p_data,
		uint64_t p_total_size,
		uint64_t *r_offset,
		uint32_t *r_value) const {
	if (p_data == nullptr || r_offset == nullptr || r_value == nullptr) {
		return false;
	}

	if (*r_offset > p_total_size ||
			p_total_size - *r_offset < 4) {
		return false;
	}

	const uint64_t offset = *r_offset;

	*r_value =
			static_cast<uint32_t>(p_data[offset]) |
			(static_cast<uint32_t>(p_data[offset + 1]) << 8) |
			(static_cast<uint32_t>(p_data[offset + 2]) << 16) |
			(static_cast<uint32_t>(p_data[offset + 3]) << 24);

	*r_offset += 4;
	return true;
}

bool PackedLuaArchive::read_u64(
		const uint8_t *p_data,
		uint64_t p_total_size,
		uint64_t *r_offset,
		uint64_t *r_value) const {
	if (p_data == nullptr || r_offset == nullptr || r_value == nullptr) {
		return false;
	}

	if (*r_offset > p_total_size ||
			p_total_size - *r_offset < 8) {
		return false;
	}

	uint64_t value = 0;
	const uint64_t offset = *r_offset;

	for (uint32_t i = 0; i < 8; i++) {
		value |= static_cast<uint64_t>(p_data[offset + i]) << (i * 8);
	}

	*r_value = value;
	*r_offset += 8;
	return true;
}

bool PackedLuaArchive::read_module_name(
		const uint8_t *p_data,
		uint64_t p_total_size,
		uint64_t *r_offset,
		godot::String *r_module_name) const {
	if (p_data == nullptr || r_offset == nullptr || r_module_name == nullptr) {
		return false;
	}

	uint32_t name_size = 0;
	if (!read_u32(p_data, p_total_size, r_offset, &name_size)) {
		return false;
	}

	if (name_size == 0 || name_size > MAX_MODULE_NAME_SIZE) {
		return false;
	}

	if (*r_offset > p_total_size ||
			name_size > p_total_size - *r_offset) {
		return false;
	}

	const uint64_t offset = *r_offset;
	const uint8_t *name_ptr = p_data + offset;

	for (uint32_t i = 0; i < name_size; i++) {
		if (name_ptr[i] == 0) {
			return false;
		}
	}

	godot::String module_name = godot::String::utf8(
			reinterpret_cast<const char *>(name_ptr),
			static_cast<int>(name_size));

	if (module_name.is_empty()) {
		return false;
	}

	if (module_name.contains("/") ||
			module_name.contains("\\") ||
			module_name.begins_with(".")) {
		return false;
	}

	*r_module_name = module_name;
	*r_offset += name_size;
	return true;
}

} // namespace luagd