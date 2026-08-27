#include "res_module.h"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 已加载资源的存储
static godot::HashMap<godot::String, godot::Ref<godot::Resource>> loaded_resources;

// native_res.load(path) -> bool
// 预加载资源。
// path: 资源路径，如 "res://assets/char/sm_char_proto.glb"
// 返回：是否加载成功。
static int l_load(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);
	godot::String res_path(path);

	// 检查是否已加载
	if (loaded_resources.has(res_path)) {
		lua_pushboolean(p_L, true);
		return 1;
	}

	// 加载资源
	godot::Ref<godot::Resource> resource = godot::ResourceLoader::get_singleton()->load(res_path);
	if (resource.is_null()) {
		godot::UtilityFunctions::printerr("native_res.load: failed to load resource: ", path);
		lua_pushboolean(p_L, false);
		return 1;
	}

	// 存储资源引用
	loaded_resources[res_path] = resource;
	lua_pushboolean(p_L, true);
	return 1;
}

// native_res.unload(path) -> void
// 卸载资源，释放引用。
// path: 资源路径
static int l_unload(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);
	godot::String res_path(path);

	// 从 HashMap 中移除（Ref 析构时自动释放引用）
	if (loaded_resources.has(res_path)) {
		loaded_resources.erase(res_path);
	}

	return 0;
}

// native_res.is_loaded(path) -> bool
// 检查资源是否已加载。
// path: 资源路径
static int l_is_loaded(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);
	godot::String res_path(path);

	lua_pushboolean(p_L, loaded_resources.has(res_path));
	return 1;
}

// 获取 Texture2D 资源。
// 优先复用已预加载的缓存，否则临时加载；临时加载不写入缓存。
// 返回：null 表示加载失败或类型不是 Texture2D。
static godot::Ref<godot::Texture2D> get_texture(const godot::String &p_path) {
	if (loaded_resources.has(p_path)) {
		return godot::Ref<godot::Texture2D>(godot::Object::cast_to<godot::Texture2D>(loaded_resources[p_path].ptr()));
	}

	godot::Ref<godot::Resource> resource = godot::ResourceLoader::get_singleton()->load(p_path);
	if (resource.is_null()) {
		return nullptr;
	}

	return godot::Ref<godot::Texture2D>(godot::Object::cast_to<godot::Texture2D>(resource.ptr()));
}

// native_res.get_texture_width(path) -> integer
// 获取纹理宽度（像素）。加载失败或非纹理资源时返回 0。
// path: 资源路径
static int l_get_texture_width(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);
	godot::Ref<godot::Texture2D> texture = get_texture(godot::String(path));
	if (texture.is_null()) {
		godot::UtilityFunctions::printerr("native_res.get_texture_width: not a valid texture: ", path);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	lua_pushinteger(p_L, texture->get_width());
	return 1;
}

// native_res.get_texture_height(path) -> integer
// 获取纹理图像的高度（像素）。path 或非 Texture2D 资源时返回 0。
// path: 资源路径
static int l_get_texture_height(lua_State *p_L) {
	const char *path = luaL_checkstring(p_L, 1);
	godot::Ref<godot::Texture2D> texture = get_texture(godot::String(path));
	if (texture.is_null()) {
		godot::UtilityFunctions::printerr("native_res.get_texture_height: not a valid texture: ", path);
		lua_pushinteger(p_L, 0);
		return 1;
	}

	lua_pushinteger(p_L, texture->get_height());
	return 1;
}

static const luaL_Reg res_funcs[] = {
	{"load", l_load},
	{"unload", l_unload},
	{"is_loaded", l_is_loaded},
	{"get_texture_width", l_get_texture_width},
	{"get_texture_height", l_get_texture_height},
	{nullptr, nullptr}
};

int luaopen_native_res(lua_State *p_L) {
	luaL_newlib(p_L, res_funcs);
	return 1;
}

void res_cleanup() {
	loaded_resources.clear();
}

} // namespace luagd
