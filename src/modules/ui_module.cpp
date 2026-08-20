#include "ui_module.h"

#include "node_module.h"

#include <godot_cpp/classes/canvas_item.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/range.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 从 Lua 栈读取 ObjectID 句柄。
static godot::ObjectID _read_object_id(lua_State *p_L, int p_index) {
	return godot::ObjectID((uint64_t)luaL_checkinteger(p_L, p_index));
}

// 解析 CanvasItem 节点。
// 返回：节点对象；句柄为空、对象不存在或类型不符时返回 nullptr。
static godot::CanvasItem *_resolve_canvas_item(godot::ObjectID p_id, const char *p_func_name) {
	if (p_id.is_null()) {
		godot::UtilityFunctions::printerr("native_ui.", p_func_name, ": handle is null");
		return nullptr;
	}

	godot::Node *node = node_resolve_any(p_id);
	if (node == nullptr) {
		godot::UtilityFunctions::printerr("native_ui.", p_func_name, ": node is no longer valid, handle=", (uint64_t)p_id);
		return nullptr;
	}

	godot::CanvasItem *canvas_item = godot::Object::cast_to<godot::CanvasItem>(node);
	if (canvas_item == nullptr) {
		godot::UtilityFunctions::printerr("native_ui.", p_func_name, ": node is not a CanvasItem, handle=", (uint64_t)p_id);
		return nullptr;
	}

	return canvas_item;
}

// 解析 Control 节点。
// 返回：节点对象；句柄为空、对象不存在或类型不符时返回 nullptr。
static godot::Control *_resolve_control(godot::ObjectID p_id, const char *p_func_name) {
	godot::CanvasItem *canvas_item = _resolve_canvas_item(p_id, p_func_name);
	if (canvas_item == nullptr) {
		return nullptr;
	}

	godot::Control *control = godot::Object::cast_to<godot::Control>(canvas_item);
	if (control == nullptr) {
		godot::UtilityFunctions::printerr("native_ui.", p_func_name, ": object is not a Control, handle=", (uint64_t)p_id);
		return nullptr;
	}

	return control;
}

// 解析 Range 节点。
// 返回：节点对象；句柄为空、对象不存在或类型不符时返回 nullptr。
static godot::Range *_resolve_range(godot::ObjectID p_id, const char *p_func_name) {
	godot::Control *control = _resolve_control(p_id, p_func_name);
	if (control == nullptr) {
		return nullptr;
	}

	godot::Range *range = godot::Object::cast_to<godot::Range>(control);
	if (range == nullptr) {
		godot::UtilityFunctions::printerr("native_ui.", p_func_name, ": object is not a Range, handle=", (uint64_t)p_id);
		return nullptr;
	}

	return range;
}

// 解析 RichTextLabel 节点。
// 返回：节点对象；句柄为空、对象不存在或类型不符时返回 nullptr。
static godot::RichTextLabel *_resolve_rich_text_label(godot::ObjectID p_id, const char *p_func_name) {
	godot::Control *control = _resolve_control(p_id, p_func_name);
	if (control == nullptr) {
		return nullptr;
	}

	godot::RichTextLabel *rich_text_label = godot::Object::cast_to<godot::RichTextLabel>(control);
	if (rich_text_label == nullptr) {
		godot::UtilityFunctions::printerr("native_ui.", p_func_name, ": object is not a RichTextLabel, handle=", (uint64_t)p_id);
		return nullptr;
	}

	return rich_text_label;
}

// get_visible(handle) -> bool
// 获取 CanvasItem 的可见性。
// 返回：节点无效时返回 false。
static int l_get_visible(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);

	godot::CanvasItem *canvas_item = _resolve_canvas_item(id, "get_visible");
	if (canvas_item == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	lua_pushboolean(p_L, canvas_item->is_visible());
	return 1;
}

// set_visible(handle, visible) -> void
// 设置 CanvasItem 的可见性。
static int l_set_visible(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_ui.set_visible: expected 2 args (handle, visible), got ", argc);
		return 0;
	}

	const godot::ObjectID id = _read_object_id(p_L, 1);
	const bool visible = lua_toboolean(p_L, 2);

	godot::CanvasItem *canvas_item = _resolve_canvas_item(id, "set_visible");
	if (canvas_item == nullptr) {
		return 0;
	}

	canvas_item->set_visible(visible);
	return 0;
}

// set_modulate(handle, r, g, b, a) -> void
// 设置 CanvasItem 的调制颜色。
// 约束：r、g、b、a 为 [0.0, 1.0] 内的数值。
static int l_set_modulate(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 5) {
		godot::UtilityFunctions::printerr("native_ui.set_modulate: expected 5 args (handle, r, g, b, a), got ", argc);
		return 0;
	}

	const godot::ObjectID id = _read_object_id(p_L, 1);
	const double r = luaL_checknumber(p_L, 2);
	const double g = luaL_checknumber(p_L, 3);
	const double b = luaL_checknumber(p_L, 4);
	const double a = luaL_checknumber(p_L, 5);

	godot::CanvasItem *canvas_item = _resolve_canvas_item(id, "set_modulate");
	if (canvas_item == nullptr) {
		return 0;
	}

	godot::Color color((float)r, (float)g, (float)b, (float)a);
	canvas_item->set_modulate(color);
	return 0;
}

// get_bar_value(handle) -> number
// 获取 Range 的值。
// 返回：节点无效时返回 0.0。
static int l_get_bar_value(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);

	godot::Range *range = _resolve_range(id, "get_bar_value");
	if (range == nullptr) {
		lua_pushnumber(p_L, 0.0);
		return 1;
	}

	lua_pushnumber(p_L, range->get_value());
	return 1;
}

// set_bar_value(handle, value) -> void
// 设置 Range 的值。
// 注意：会触发 value_changed 信号。
static int l_set_bar_value(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_ui.set_bar_value: expected 2 args (handle, value), got ", argc);
		return 0;
	}

	const godot::ObjectID id = _read_object_id(p_L, 1);
	const double value = luaL_checknumber(p_L, 2);

	godot::Range *range = _resolve_range(id, "set_bar_value");
	if (range == nullptr) {
		return 0;
	}

	range->set_value(value);
	return 0;
}

// get_size(handle) -> number, number
// 获取 Control 节点的尺寸（宽度与高度）。
// 返回：节点无效时返回 0.0, 0.0。
static int l_get_size(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);

	godot::Control *control = _resolve_control(id, "get_size");
	if (control == nullptr) {
		lua_pushnumber(p_L, 0.0);
		lua_pushnumber(p_L, 0.0);
		return 2;
	}

	const godot::Vector2 size = control->get_size();
	lua_pushnumber(p_L, size.x);
	lua_pushnumber(p_L, size.y);
	return 2;
}

// get_text(handle) -> string
// 获取 RichTextLabel 的文本内容。
// 返回：节点无效时返回空字符串。
static int l_get_text(lua_State *p_L) {
	const godot::ObjectID id = _read_object_id(p_L, 1);

	godot::RichTextLabel *rich_text_label = _resolve_rich_text_label(id, "get_text");
	if (rich_text_label == nullptr) {
		lua_pushstring(p_L, "");
		return 1;
	}

	const godot::String text = rich_text_label->get_text();
	lua_pushstring(p_L, text.utf8().get_data());
	return 1;
}

// set_text(handle, text) -> void
// 设置 RichTextLabel 的文本内容。
static int l_set_text(lua_State *p_L) {
	int argc = lua_gettop(p_L);
	if (argc < 2) {
		godot::UtilityFunctions::printerr("native_ui.set_text: expected 2 args (handle, text), got ", argc);
		return 0;
	}

	const godot::ObjectID id = _read_object_id(p_L, 1);
	const char *text = luaL_checkstring(p_L, 2);

	godot::RichTextLabel *rich_text_label = _resolve_rich_text_label(id, "set_text");
	if (rich_text_label == nullptr) {
		return 0;
	}

	rich_text_label->set_text(godot::String::utf8(text));
	return 0;
}

static const luaL_Reg ui_funcs[] = {
	{"get_visible", l_get_visible},
	{"set_visible", l_set_visible},
	{"set_modulate", l_set_modulate},
	{"get_bar_value", l_get_bar_value},
	{"set_bar_value", l_set_bar_value},
	{"get_size", l_get_size},
	{"get_text", l_get_text},
	{"set_text", l_set_text},
	{nullptr, nullptr}
};

int luaopen_native_ui(lua_State *p_L) {
	luaL_newlib(p_L, ui_funcs);
	return 1;
}

} // namespace luagd
