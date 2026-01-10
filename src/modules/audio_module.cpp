#include "audio_module.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_mp3.hpp>
#include <godot_cpp/classes/audio_stream_ogg_vorbis.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

// 播放器记录
struct PlayerRecord {
	int32_t id;
	bool is_spatial;
	godot::AudioStreamPlayer *player;
	godot::AudioStreamPlayer3D *player_3d;
};

// 模块级静态数据
static godot::Node *audio_root = nullptr;
static godot::HashMap<int32_t, PlayerRecord> players;
static int32_t next_id = 1;
static bool initialized = false;

// 获取播放器记录，不存在时打印错误
static PlayerRecord *get_player(int32_t p_id, const char *p_func_name) {
	if (!players.has(p_id)) {
		godot::UtilityFunctions::printerr("native_audio.", p_func_name, ": invalid id ", p_id);
		return nullptr;
	}
	return &players[p_id];
}

// ============================================================================
// 模块初始化
// ============================================================================

// init(root_name) -> bool
// 初始化音频模块，创建容器节点。
// 必须在场景树就绪后调用（如第一帧 update 中）。
static int l_init(lua_State *p_L) {
	if (initialized) {
		lua_pushboolean(p_L, true);
		return 1;
	}

	const char *root_name = luaL_optstring(p_L, 1, "_AudioRoot");

	godot::SceneTree *tree = godot::Object::cast_to<godot::SceneTree>(
		godot::Engine::get_singleton()->get_main_loop()
	);
	if (tree == nullptr) {
		godot::UtilityFunctions::printerr("native_audio.init: SceneTree not available");
		lua_pushboolean(p_L, false);
		return 1;
	}

	audio_root = memnew(godot::Node);
	audio_root->set_name(root_name);
	tree->get_root()->add_child(audio_root);
	initialized = true;

	lua_pushboolean(p_L, true);
	return 1;
}

// is_initialized() -> bool
// 查询模块是否已初始化。
static int l_is_initialized(lua_State *p_L) {
	lua_pushboolean(p_L, initialized);
	return 1;
}

// ============================================================================
// 播放器生命周期
// ============================================================================

// create_player(is_spatial) -> id
// 创建播放器，挂载到场景树。
// is_spatial: false=AudioStreamPlayer, true=AudioStreamPlayer3D
// 返回播放器 ID，失败返回 -1。
static int l_create_player(lua_State *p_L) {
	if (!initialized) {
		godot::UtilityFunctions::printerr("native_audio.create_player: not initialized, call init() first");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	bool is_spatial = lua_toboolean(p_L, 1);

	PlayerRecord rec;
	rec.id = next_id++;
	rec.is_spatial = is_spatial;

	if (is_spatial) {
		rec.player_3d = memnew(godot::AudioStreamPlayer3D);
		rec.player = nullptr;
		audio_root->add_child(rec.player_3d);
	} else {
		rec.player = memnew(godot::AudioStreamPlayer);
		rec.player_3d = nullptr;
		audio_root->add_child(rec.player);
	}

	players[rec.id] = rec;
	lua_pushinteger(p_L, rec.id);
	return 1;
}

// destroy_player(id) -> void
// 销毁播放器，从场景树移除。
static int l_destroy_player(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	PlayerRecord *rec = get_player(id, "destroy_player");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->is_spatial) {
		rec->player_3d->queue_free();
	} else {
		rec->player->queue_free();
	}
	players.erase(id);
	return 0;
}

// ============================================================================
// 播放器操作
// ============================================================================

// set_stream(id, path) -> bool
// 加载音频资源到播放器。
static int l_set_stream(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *path = luaL_checkstring(p_L, 2);

	PlayerRecord *rec = get_player(id, "set_stream");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	godot::Ref<godot::AudioStream> stream = godot::ResourceLoader::get_singleton()->load(path);
	if (stream.is_null()) {
		godot::UtilityFunctions::printerr("native_audio.set_stream: failed to load ", path);
		lua_pushboolean(p_L, false);
		return 1;
	}

	if (rec->is_spatial) {
		rec->player_3d->set_stream(stream);
	} else {
		rec->player->set_stream(stream);
	}

	lua_pushboolean(p_L, true);
	return 1;
}

// play(id) -> void
// 播放音频。
static int l_play(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	PlayerRecord *rec = get_player(id, "play");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->is_spatial) {
		rec->player_3d->play();
	} else {
		rec->player->play();
	}
	return 0;
}

// stop(id) -> void
// 停止播放。
static int l_stop(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	PlayerRecord *rec = get_player(id, "stop");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->is_spatial) {
		rec->player_3d->stop();
	} else {
		rec->player->stop();
	}
	return 0;
}

// is_playing(id) -> bool
// 查询是否正在播放。
static int l_is_playing(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	PlayerRecord *rec = get_player(id, "is_playing");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	bool playing = rec->is_spatial ? rec->player_3d->is_playing() : rec->player->is_playing();
	lua_pushboolean(p_L, playing);
	return 1;
}

// set_paused(id, paused) -> void
// 设置播放器暂停状态。
static int l_set_paused(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	bool paused = lua_toboolean(p_L, 2);

	PlayerRecord *rec = get_player(id, "set_paused");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->is_spatial) {
		rec->player_3d->set_stream_paused(paused);
	} else {
		rec->player->set_stream_paused(paused);
	}
	return 0;
}

// is_paused(id) -> bool
// 查询播放器是否暂停。
static int l_is_paused(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);

	PlayerRecord *rec = get_player(id, "is_paused");
	if (rec == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	bool paused = rec->is_spatial ? rec->player_3d->get_stream_paused() : rec->player->get_stream_paused();
	lua_pushboolean(p_L, paused);
	return 1;
}

// set_volume(id, volume) -> void
// 设置音量（线性 0-1）。
static int l_set_volume(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double volume = luaL_checknumber(p_L, 2);

	PlayerRecord *rec = get_player(id, "set_volume");
	if (rec == nullptr) {
		return 0;
	}

	float db = (float)godot::UtilityFunctions::linear_to_db(volume);
	if (rec->is_spatial) {
		rec->player_3d->set_volume_db(db);
	} else {
		rec->player->set_volume_db(db);
	}
	return 0;
}

// set_pitch(id, pitch) -> void
// 设置音调倍率。
static int l_set_pitch(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double pitch = luaL_checknumber(p_L, 2);

	PlayerRecord *rec = get_player(id, "set_pitch");
	if (rec == nullptr) {
		return 0;
	}

	if (rec->is_spatial) {
		rec->player_3d->set_pitch_scale((float)pitch);
	} else {
		rec->player->set_pitch_scale((float)pitch);
	}
	return 0;
}

// set_bus(id, bus_name) -> void
// 设置播放器的 AudioBus。
static int l_set_bus(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *bus_name = luaL_checkstring(p_L, 2);

	PlayerRecord *rec = get_player(id, "set_bus");
	if (rec == nullptr) {
		return 0;
	}

	godot::StringName bus(bus_name);
	if (rec->is_spatial) {
		rec->player_3d->set_bus(bus);
	} else {
		rec->player->set_bus(bus);
	}
	return 0;
}

// set_position(id, x, y, z) -> void
// 设置空间音频位置（仅对空间播放器有效）。
static int l_set_position(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	double x = luaL_checknumber(p_L, 2);
	double y = luaL_checknumber(p_L, 3);
	double z = luaL_checknumber(p_L, 4);

	PlayerRecord *rec = get_player(id, "set_position");
	if (rec == nullptr) {
		return 0;
	}

	if (!rec->is_spatial) {
		godot::UtilityFunctions::printerr("native_audio.set_position: player is not spatial, id ", id);
		return 0;
	}

	rec->player_3d->set_position(godot::Vector3((float)x, (float)y, (float)z));
	return 0;
}

// set_loop(id, enabled) -> void
// 设置音频循环播放。
static int l_set_loop(lua_State *p_L) {
	int32_t id = (int32_t)luaL_checkinteger(p_L, 1);
	bool enabled = lua_toboolean(p_L, 2);

	PlayerRecord *rec = get_player(id, "set_loop");
	if (rec == nullptr) {
		return 0;
	}

	godot::Ref<godot::AudioStream> stream;
	if (rec->is_spatial) {
		stream = rec->player_3d->get_stream();
	} else {
		stream = rec->player->get_stream();
	}

	if (stream.is_null()) {
		godot::UtilityFunctions::printerr("native_audio.set_loop: no stream set, id ", id);
		return 0;
	}

	// 尝试 MP3
	godot::AudioStreamMP3 *mp3 = godot::Object::cast_to<godot::AudioStreamMP3>(stream.ptr());
	if (mp3 != nullptr) {
		mp3->set_loop(enabled);
		return 0;
	}

	// 尝试 OGG
	godot::AudioStreamOggVorbis *ogg = godot::Object::cast_to<godot::AudioStreamOggVorbis>(stream.ptr());
	if (ogg != nullptr) {
		ogg->set_loop(enabled);
		return 0;
	}

	godot::UtilityFunctions::printerr("native_audio.set_loop: unsupported stream type, id ", id);
	return 0;
}

// ============================================================================
// AudioBus 操作
// ============================================================================

// add_bus(name) -> int
// 添加新的 AudioBus，发送到 Master。
// 返回 bus index，失败返回 -1。
static int l_add_bus(lua_State *p_L) {
	const char *name = luaL_checkstring(p_L, 1);

	godot::AudioServer *audio_server = godot::AudioServer::get_singleton();
	if (audio_server == nullptr) {
		godot::UtilityFunctions::printerr("native_audio.add_bus: AudioServer not available");
		lua_pushinteger(p_L, -1);
		return 1;
	}

	// 检查是否已存在
	int32_t existing_idx = audio_server->get_bus_index(name);
	if (existing_idx >= 0) {
		lua_pushinteger(p_L, existing_idx);
		return 1;
	}

	// 添加新 bus
	audio_server->add_bus(-1);
	int32_t bus_idx = audio_server->get_bus_count() - 1;
	audio_server->set_bus_name(bus_idx, name);
	audio_server->set_bus_send(bus_idx, "Master");

	lua_pushinteger(p_L, bus_idx);
	return 1;
}

// set_bus_volume(name, volume) -> void
// 设置 AudioBus 音量（线性 0-1）。
static int l_set_bus_volume(lua_State *p_L) {
	const char *name = luaL_checkstring(p_L, 1);
	double volume = luaL_checknumber(p_L, 2);

	godot::AudioServer *audio_server = godot::AudioServer::get_singleton();
	if (audio_server == nullptr) {
		godot::UtilityFunctions::printerr("native_audio.set_bus_volume: AudioServer not available");
		return 0;
	}

	int32_t bus_idx = audio_server->get_bus_index(name);
	if (bus_idx < 0) {
		godot::UtilityFunctions::printerr("native_audio.set_bus_volume: bus not found: ", name);
		return 0;
	}

	float db = (float)godot::UtilityFunctions::linear_to_db(volume);
	audio_server->set_bus_volume_db(bus_idx, db);
	return 0;
}

// get_bus_volume(name) -> number
// 获取 AudioBus 音量（线性 0-1）。
static int l_get_bus_volume(lua_State *p_L) {
	const char *name = luaL_checkstring(p_L, 1);

	godot::AudioServer *audio_server = godot::AudioServer::get_singleton();
	if (audio_server == nullptr) {
		godot::UtilityFunctions::printerr("native_audio.get_bus_volume: AudioServer not available");
		lua_pushnumber(p_L, 0.0);
		return 1;
	}

	int32_t bus_idx = audio_server->get_bus_index(name);
	if (bus_idx < 0) {
		godot::UtilityFunctions::printerr("native_audio.get_bus_volume: bus not found: ", name);
		lua_pushnumber(p_L, 0.0);
		return 1;
	}

	float db = audio_server->get_bus_volume_db(bus_idx);
	double linear = godot::UtilityFunctions::db_to_linear(db);
	lua_pushnumber(p_L, linear);
	return 1;
}

// set_master_volume(volume) -> void
// 设置主音量（线性 0-1）。
static int l_set_master_volume(lua_State *p_L) {
	double volume = luaL_checknumber(p_L, 1);

	godot::AudioServer *audio_server = godot::AudioServer::get_singleton();
	if (audio_server == nullptr) {
		godot::UtilityFunctions::printerr("native_audio.set_master_volume: AudioServer not available");
		return 0;
	}

	float db = (float)godot::UtilityFunctions::linear_to_db(volume);
	audio_server->set_bus_volume_db(0, db);
	return 0;
}

// get_master_volume() -> number
// 获取主音量（线性 0-1）。
static int l_get_master_volume(lua_State *p_L) {
	godot::AudioServer *audio_server = godot::AudioServer::get_singleton();
	if (audio_server == nullptr) {
		godot::UtilityFunctions::printerr("native_audio.get_master_volume: AudioServer not available");
		lua_pushnumber(p_L, 0.0);
		return 1;
	}

	float db = audio_server->get_bus_volume_db(0);
	double linear = godot::UtilityFunctions::db_to_linear(db);
	lua_pushnumber(p_L, linear);
	return 1;
}

// ============================================================================
// 模块注册
// ============================================================================

static const luaL_Reg audio_funcs[] = {
	// 模块初始化
	{"init", l_init},
	{"is_initialized", l_is_initialized},

	// 播放器生命周期
	{"create_player", l_create_player},
	{"destroy_player", l_destroy_player},

	// 播放器操作
	{"set_stream", l_set_stream},
	{"play", l_play},
	{"stop", l_stop},
	{"is_playing", l_is_playing},
	{"set_paused", l_set_paused},
	{"is_paused", l_is_paused},
	{"set_volume", l_set_volume},
	{"set_pitch", l_set_pitch},
	{"set_bus", l_set_bus},
	{"set_position", l_set_position},
	{"set_loop", l_set_loop},

	// AudioBus 操作
	{"add_bus", l_add_bus},
	{"set_bus_volume", l_set_bus_volume},
	{"get_bus_volume", l_get_bus_volume},
	{"set_master_volume", l_set_master_volume},
	{"get_master_volume", l_get_master_volume},

	{nullptr, nullptr}
};

int luaopen_native_audio(lua_State *p_L) {
	luaL_newlib(p_L, audio_funcs);
	return 1;
}

void audio_cleanup() {
	// 销毁所有播放器
	for (const godot::KeyValue<int32_t, PlayerRecord> &kv : players) {
		const PlayerRecord &rec = kv.value;
		if (rec.is_spatial) {
			if (rec.player_3d != nullptr) {
				rec.player_3d->queue_free();
			}
		} else {
			if (rec.player != nullptr) {
				rec.player->queue_free();
			}
		}
	}
	players.clear();

	// 销毁容器节点
	if (audio_root != nullptr) {
		audio_root->queue_free();
		audio_root = nullptr;
	}

	next_id = 1;
	initialized = false;
}

} // namespace luagd
