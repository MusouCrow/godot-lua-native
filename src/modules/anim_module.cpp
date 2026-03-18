#include "anim_module.h"

#include "../host/host_thread_check.h"
#include "node_module.h"

#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/animation_mixer.hpp>
#include <godot_cpp/classes/animation_node.hpp>
#include <godot_cpp/classes/animation_node_add2.hpp>
#include <godot_cpp/classes/animation_node_animation.hpp>
#include <godot_cpp/classes/animation_node_blend2.hpp>
#include <godot_cpp/classes/animation_node_blend_space2d.hpp>
#include <godot_cpp/classes/animation_node_blend_tree.hpp>
#include <godot_cpp/classes/animation_node_time_scale.hpp>
#include <godot_cpp/classes/animation_player.hpp>
#include <godot_cpp/classes/animation_tree.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/vector2.hpp>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

namespace luagd {

enum MixMode {
	MIX_BLEND = 0,
	MIX_ADD = 1,
};

enum LayerFlags {
	FLAG_NONE = 0,
	FLAG_ALLOW_BLEND2D = 1 << 0,
};

enum SourceKind {
	SOURCE_NONE = 0,
	SOURCE_ANIM = 1,
	SOURCE_BLEND2D = 2,
};

enum SlotIndex {
	SLOT_A = 0,
	SLOT_B = 1,
};

static const char *EMPTY_ANIM_NAME = "__native_anim_empty";
static const char *BASE_NODE_NAME = "__native_anim_base";
static const int32_t INVALID_ANIMATOR_ID = -1;

struct Blend2DPointRecord {
	godot::StringName anim_name;
	godot::Vector2 position;
};

struct SlotRecord {
	int32_t source_kind;
	bool playing;
	godot::StringName anim_name;
	godot::StringName source_node_name;
	godot::StringName time_scale_node_name;
};

struct LayerRecord {
	godot::StringName name;
	int32_t mix_mode;
	int32_t order;
	int32_t flags;
	double weight;
	double speed;
	int32_t active_slot;
	double current_blend;
	double target_blend;
	double fade_duration;
	double fade_elapsed;
	bool fading;
	SlotRecord slot_a;
	SlotRecord slot_b;
	godot::StringName slot_switch_node_name;
	godot::StringName layer_mix_node_name;
	godot::Vector<Blend2DPointRecord> blend2d_points;
	double blend2d_x;
	double blend2d_y;
};

struct AnimatorRecord {
	int32_t id;
	int32_t owner_node_id;
	godot::NodePath animation_player_path;
	godot::AnimationTree *animation_tree;
	godot::Ref<godot::AnimationNodeBlendTree> tree_root;
	godot::HashMap<godot::StringName, LayerRecord> layers;
	godot::Vector<godot::StringName> layer_order;
};

static godot::HashMap<int32_t, AnimatorRecord> animators;
static int32_t next_animator_id = 1;

// 统一处理 native_anim 的主线程约束。
static bool _ensure_main_thread(const char *p_func_name) {
	return ensure_main_thread(godot::String("native_anim.") + p_func_name);
}

static godot::String _param_path(const godot::StringName &p_node_name, const char *p_property) {
	return godot::String("parameters/") + godot::String(p_node_name) + "/" + p_property;
}

static void _push_bool(lua_State *p_L, bool p_value) {
	lua_pushboolean(p_L, p_value);
}

static bool _is_valid_mix_mode(int32_t p_mix_mode) {
	return p_mix_mode == MIX_BLEND || p_mix_mode == MIX_ADD;
}

// 外部 AnimationPlayer 不归 native_anim 持有，每次使用时重新解析。
static godot::Node3D *_resolve_owner_node(const AnimatorRecord &p_animator) {
	return node_resolve(p_animator.owner_node_id);
}

static godot::AnimationPlayer *_resolve_animation_player(const AnimatorRecord &p_animator) {
	if (p_animator.animation_player_path.is_empty()) {
		return nullptr;
	}

	godot::Node3D *owner = _resolve_owner_node(p_animator);
	if (owner == nullptr) {
		return nullptr;
	}

	godot::Node *node = owner->get_node<godot::Node>(p_animator.animation_player_path);
	if (node == nullptr) {
		return nullptr;
	}

	return godot::Object::cast_to<godot::AnimationPlayer>(node);
}

static godot::AnimationPlayer *_find_animation_player_recursive(godot::Node *p_node) {
	if (p_node == nullptr) {
		return nullptr;
	}

	godot::AnimationPlayer *player = godot::Object::cast_to<godot::AnimationPlayer>(p_node);
	if (player != nullptr) {
		return player;
	}

	int32_t child_count = p_node->get_child_count();
	for (int32_t i = 0; i < child_count; i++) {
		godot::Node *child = p_node->get_child(i);
		godot::AnimationPlayer *found = _find_animation_player_recursive(child);
		if (found != nullptr) {
			return found;
		}
	}

	return nullptr;
}

static bool _ensure_empty_animation(godot::AnimationPlayer *p_player) {
	if (p_player == nullptr) {
		return false;
	}

	if (p_player->has_animation(EMPTY_ANIM_NAME)) {
		return true;
	}

	godot::Ref<godot::AnimationLibrary> library = p_player->get_animation_library("");
	if (library.is_null()) {
		library.instantiate();
		if (p_player->add_animation_library("", library) != godot::OK) {
			godot::UtilityFunctions::printerr("native_anim.create_animator: failed to add global animation library");
			return false;
		}
	}

	if (library->has_animation(EMPTY_ANIM_NAME)) {
		return true;
	}

	godot::Ref<godot::Animation> animation;
	animation.instantiate();
	animation->set_length(0.01);
	animation->set_loop_mode(godot::Animation::LOOP_NONE);
	library->add_animation(EMPTY_ANIM_NAME, animation);
	return true;
}

static bool _is_animator_runtime_valid(const AnimatorRecord &p_animator) {
	if (_resolve_owner_node(p_animator) == nullptr) {
		return false;
	}
	if (_resolve_animation_player(p_animator) == nullptr) {
		return false;
	}
	if (p_animator.animation_tree == nullptr || !p_animator.animation_tree->is_inside_tree()) {
		return false;
	}
	if (p_animator.tree_root.is_null()) {
		return false;
	}
	return true;
}

static godot::AnimationPlayer *_get_animation_player(AnimatorRecord *p_animator, const char *p_func_name) {
	if (p_animator == nullptr) {
		return nullptr;
	}

	godot::AnimationPlayer *player = _resolve_animation_player(*p_animator);
	if (player == nullptr) {
		godot::UtilityFunctions::printerr("native_anim.", p_func_name, ": animation player is no longer valid");
		return nullptr;
	}

	return player;
}

static AnimatorRecord *_get_animator(int32_t p_animator_id, const char *p_func_name) {
	if (!animators.has(p_animator_id)) {
		godot::UtilityFunctions::printerr("native_anim.", p_func_name, ": invalid animator id ", p_animator_id);
		return nullptr;
	}

	AnimatorRecord *animator = &animators[p_animator_id];
	if (!_is_animator_runtime_valid(*animator)) {
		godot::UtilityFunctions::printerr("native_anim.", p_func_name, ": animator is no longer valid, id ", p_animator_id);
		return nullptr;
	}

	return animator;
}

static LayerRecord *_get_layer(AnimatorRecord *p_animator, const godot::StringName &p_layer_name, const char *p_func_name) {
	if (p_animator == nullptr) {
		return nullptr;
	}
	if (!p_animator->layers.has(p_layer_name)) {
		godot::UtilityFunctions::printerr("native_anim.", p_func_name, ": layer not found: ", godot::String(p_layer_name));
		return nullptr;
	}
	return &p_animator->layers[p_layer_name];
}

static void _set_tree_parameter(godot::AnimationTree *p_tree, const godot::StringName &p_node_name, const char *p_property, const godot::Variant &p_value) {
	if (p_tree == nullptr) {
		return;
	}
	p_tree->set(_param_path(p_node_name, p_property), p_value);
}

static void _set_layer_weight_runtime(AnimatorRecord *p_animator, LayerRecord *p_layer) {
	if (p_animator == nullptr || p_layer == nullptr) {
		return;
	}
	_set_tree_parameter(p_animator->animation_tree, p_layer->layer_mix_node_name, "blend_amount", p_layer->weight);
}

static void _set_slot_speed_runtime(AnimatorRecord *p_animator, LayerRecord *p_layer, SlotRecord *p_slot) {
	if (p_animator == nullptr || p_layer == nullptr || p_slot == nullptr) {
		return;
	}
	_set_tree_parameter(p_animator->animation_tree, p_slot->time_scale_node_name, "scale", p_layer->speed);
}

static void _set_slot_blend_position_runtime(AnimatorRecord *p_animator, LayerRecord *p_layer, SlotRecord *p_slot) {
	if (p_animator == nullptr || p_layer == nullptr || p_slot == nullptr) {
		return;
	}
	if (p_slot->source_kind != SOURCE_BLEND2D) {
		return;
	}
	p_animator->animation_tree->set(
		_param_path(p_slot->source_node_name, "blend_position"),
		godot::Vector2((float)p_layer->blend2d_x, (float)p_layer->blend2d_y)
	);
}

static void _slot_reset(SlotRecord *p_slot) {
	if (p_slot == nullptr) {
		return;
	}
	p_slot->source_kind = SOURCE_NONE;
	p_slot->playing = false;
	p_slot->anim_name = godot::StringName();
}

static void _disconnect_input(godot::Ref<godot::AnimationNodeBlendTree> p_tree_root, const godot::StringName &p_node_name, int32_t p_input_index) {
	if (p_tree_root.is_null()) {
		return;
	}
	p_tree_root->disconnect_node(p_node_name, p_input_index);
}

static void _connect_input(godot::Ref<godot::AnimationNodeBlendTree> p_tree_root, const godot::StringName &p_node_name, int32_t p_input_index, const godot::StringName &p_output_name) {
	if (p_tree_root.is_null()) {
		return;
	}
	p_tree_root->connect_node(p_node_name, p_input_index, p_output_name);
}

static godot::StringName _make_node_name(const godot::StringName &p_layer_name, const char *p_suffix) {
	return godot::StringName(godot::String("layer_") + godot::String(p_layer_name) + "_" + p_suffix);
}

static bool _remove_slot_source_node(AnimatorRecord *p_animator, SlotRecord *p_slot) {
	if (p_animator == nullptr || p_slot == nullptr) {
		return false;
	}
	if (p_slot->source_node_name.is_empty()) {
		return true;
	}
	_disconnect_input(p_animator->tree_root, p_slot->time_scale_node_name, 0);
	if (p_animator->tree_root->has_node(p_slot->source_node_name)) {
		p_animator->tree_root->remove_node(p_slot->source_node_name);
	}
	p_slot->source_node_name = godot::StringName();
	return true;
}

static bool _assign_slot_empty(AnimatorRecord *p_animator, LayerRecord *p_layer, SlotRecord *p_slot) {
	if (p_animator == nullptr || p_layer == nullptr || p_slot == nullptr) {
		return false;
	}

	_remove_slot_source_node(p_animator, p_slot);

	const godot::StringName node_name = godot::StringName(godot::String(p_slot->time_scale_node_name) + "_empty");
	godot::Ref<godot::AnimationNodeAnimation> node;
	node.instantiate();
	node->set_animation(EMPTY_ANIM_NAME);
	p_animator->tree_root->add_node(node_name, node);
	_connect_input(p_animator->tree_root, p_slot->time_scale_node_name, 0, node_name);
	p_slot->source_node_name = node_name;
	_slot_reset(p_slot);
	_set_slot_speed_runtime(p_animator, p_layer, p_slot);
	return true;
}

static bool _assign_slot_anim(AnimatorRecord *p_animator, LayerRecord *p_layer, SlotRecord *p_slot, const godot::StringName &p_anim_name) {
	if (p_animator == nullptr || p_layer == nullptr || p_slot == nullptr) {
		return false;
	}
	godot::AnimationPlayer *player = _get_animation_player(p_animator, "play");
	if (player == nullptr) {
		return false;
	}
	if (!player->has_animation(p_anim_name)) {
		godot::UtilityFunctions::printerr("native_anim.play: animation not found: ", godot::String(p_anim_name));
		return false;
	}

	_remove_slot_source_node(p_animator, p_slot);

	const godot::StringName node_name = godot::StringName(godot::String(p_slot->time_scale_node_name) + "_anim");
	godot::Ref<godot::AnimationNodeAnimation> node;
	node.instantiate();
	node->set_animation(p_anim_name);
	p_animator->tree_root->add_node(node_name, node);
	_connect_input(p_animator->tree_root, p_slot->time_scale_node_name, 0, node_name);

	p_slot->source_node_name = node_name;
	p_slot->source_kind = SOURCE_ANIM;
	p_slot->playing = true;
	p_slot->anim_name = p_anim_name;
	_set_slot_speed_runtime(p_animator, p_layer, p_slot);
	return true;
}

static bool _assign_slot_blend2d(AnimatorRecord *p_animator, LayerRecord *p_layer, SlotRecord *p_slot) {
	if (p_animator == nullptr || p_layer == nullptr || p_slot == nullptr) {
		return false;
	}
	godot::AnimationPlayer *player = _get_animation_player(p_animator, "play_blend2d");
	if (player == nullptr) {
		return false;
	}
	if ((p_layer->flags & FLAG_ALLOW_BLEND2D) == 0) {
		godot::UtilityFunctions::printerr("native_anim.play_blend2d: layer does not allow blend2d: ", godot::String(p_layer->name));
		return false;
	}
	if (p_layer->blend2d_points.is_empty()) {
		godot::UtilityFunctions::printerr("native_anim.play_blend2d: no blend2d points configured: ", godot::String(p_layer->name));
		return false;
	}

	_remove_slot_source_node(p_animator, p_slot);

	const godot::StringName node_name = godot::StringName(godot::String(p_slot->time_scale_node_name) + "_blend2d");
	godot::Ref<godot::AnimationNodeBlendSpace2D> node;
	node.instantiate();
	node->set_auto_triangles(true);
	node->set_min_space(godot::Vector2(-1.0f, -1.0f));
	node->set_max_space(godot::Vector2(1.0f, 1.0f));
	node->set_use_sync(true);

	for (int32_t i = 0; i < p_layer->blend2d_points.size(); i++) {
		const Blend2DPointRecord &point = p_layer->blend2d_points[i];
		if (!player->has_animation(point.anim_name)) {
			godot::UtilityFunctions::printerr("native_anim.play_blend2d: animation not found: ", godot::String(point.anim_name));
			return false;
		}

		godot::Ref<godot::AnimationNodeAnimation> anim_node;
		anim_node.instantiate();
		anim_node->set_animation(point.anim_name);
		node->add_blend_point(anim_node, point.position);
	}

	p_animator->tree_root->add_node(node_name, node);
	_connect_input(p_animator->tree_root, p_slot->time_scale_node_name, 0, node_name);

	p_slot->source_node_name = node_name;
	p_slot->source_kind = SOURCE_BLEND2D;
	p_slot->playing = true;
	p_slot->anim_name = godot::StringName();
	_set_slot_speed_runtime(p_animator, p_layer, p_slot);
	_set_slot_blend_position_runtime(p_animator, p_layer, p_slot);
	return true;
}

static SlotRecord *_layer_slot(LayerRecord *p_layer, int32_t p_slot_index) {
	if (p_layer == nullptr) {
		return nullptr;
	}
	return p_slot_index == SLOT_A ? &p_layer->slot_a : &p_layer->slot_b;
}

static void _start_layer_fade(AnimatorRecord *p_animator, LayerRecord *p_layer, int32_t p_target_slot, double p_fade_time) {
	if (p_animator == nullptr || p_layer == nullptr) {
		return;
	}
	p_layer->active_slot = p_target_slot;
	p_layer->target_blend = p_target_slot == SLOT_A ? 0.0 : 1.0;
	p_layer->fade_duration = p_fade_time;
	p_layer->fade_elapsed = 0.0;
	p_layer->fading = p_fade_time > 0.0;
	if (!p_layer->fading) {
		p_layer->current_blend = p_layer->target_blend;
	}
	_set_tree_parameter(p_animator->animation_tree, p_layer->slot_switch_node_name, "blend_amount", p_layer->current_blend);
	if (!p_layer->fading) {
		_set_tree_parameter(p_animator->animation_tree, p_layer->slot_switch_node_name, "blend_amount", p_layer->target_blend);
	}
}

// Layer 按 order 串接到根 BlendTree，保证混合顺序稳定。
static void _rebuild_layer_stack(AnimatorRecord *p_animator) {
	if (p_animator == nullptr || p_animator->tree_root.is_null()) {
		return;
	}

	for (int32_t i = 0; i < p_animator->layer_order.size(); i++) {
		const godot::StringName &layer_name = p_animator->layer_order[i];
		if (!p_animator->layers.has(layer_name)) {
			continue;
		}
		LayerRecord *layer = &p_animator->layers[layer_name];
		_disconnect_input(p_animator->tree_root, layer->layer_mix_node_name, 0);
	}
	_disconnect_input(p_animator->tree_root, godot::StringName("output"), 0);

	godot::StringName prev = godot::StringName(BASE_NODE_NAME);
	for (int32_t i = 0; i < p_animator->layer_order.size(); i++) {
		const godot::StringName &layer_name = p_animator->layer_order[i];
		if (!p_animator->layers.has(layer_name)) {
			continue;
		}
		LayerRecord *layer = &p_animator->layers[layer_name];
		_connect_input(p_animator->tree_root, layer->layer_mix_node_name, 0, prev);
		prev = layer->layer_mix_node_name;
	}

	_connect_input(p_animator->tree_root, godot::StringName("output"), 0, prev);
	for (int32_t i = 0; i < p_animator->layer_order.size(); i++) {
		if (p_animator->layers.has(p_animator->layer_order[i])) {
			_set_layer_weight_runtime(p_animator, &p_animator->layers[p_animator->layer_order[i]]);
		}
	}
}

static void _insert_layer_name(AnimatorRecord *p_animator, const godot::StringName &p_layer_name, int32_t p_order) {
	if (p_animator == nullptr) {
		return;
	}
	int32_t insert_index = p_animator->layer_order.size();
	for (int32_t i = 0; i < p_animator->layer_order.size(); i++) {
		const godot::StringName &other_name = p_animator->layer_order[i];
		if (!p_animator->layers.has(other_name)) {
			continue;
		}
		const LayerRecord &other = p_animator->layers[other_name];
		if (p_order < other.order) {
			insert_index = i;
			break;
		}
	}
	p_animator->layer_order.insert(insert_index, p_layer_name);
}

static void _erase_layer_name(AnimatorRecord *p_animator, const godot::StringName &p_layer_name) {
	if (p_animator == nullptr) {
		return;
	}
	for (int32_t i = 0; i < p_animator->layer_order.size(); i++) {
		if (p_animator->layer_order[i] == p_layer_name) {
			p_animator->layer_order.remove_at(i);
			return;
		}
	}
}

static bool _remove_layer_nodes(AnimatorRecord *p_animator, LayerRecord *p_layer) {
	if (p_animator == nullptr || p_layer == nullptr || p_animator->tree_root.is_null()) {
		return false;
	}
	_disconnect_input(p_animator->tree_root, p_layer->slot_a.time_scale_node_name, 0);
	_disconnect_input(p_animator->tree_root, p_layer->slot_b.time_scale_node_name, 0);
	_disconnect_input(p_animator->tree_root, p_layer->slot_switch_node_name, 0);
	_disconnect_input(p_animator->tree_root, p_layer->slot_switch_node_name, 1);
	_disconnect_input(p_animator->tree_root, p_layer->layer_mix_node_name, 0);
	_disconnect_input(p_animator->tree_root, p_layer->layer_mix_node_name, 1);

	if (p_layer->slot_a.source_node_name != godot::StringName() && p_animator->tree_root->has_node(p_layer->slot_a.source_node_name)) {
		p_animator->tree_root->remove_node(p_layer->slot_a.source_node_name);
	}
	if (p_layer->slot_b.source_node_name != godot::StringName() && p_animator->tree_root->has_node(p_layer->slot_b.source_node_name)) {
		p_animator->tree_root->remove_node(p_layer->slot_b.source_node_name);
	}
	if (p_animator->tree_root->has_node(p_layer->slot_a.time_scale_node_name)) {
		p_animator->tree_root->remove_node(p_layer->slot_a.time_scale_node_name);
	}
	if (p_animator->tree_root->has_node(p_layer->slot_b.time_scale_node_name)) {
		p_animator->tree_root->remove_node(p_layer->slot_b.time_scale_node_name);
	}
	if (p_animator->tree_root->has_node(p_layer->slot_switch_node_name)) {
		p_animator->tree_root->remove_node(p_layer->slot_switch_node_name);
	}
	if (p_animator->tree_root->has_node(p_layer->layer_mix_node_name)) {
		p_animator->tree_root->remove_node(p_layer->layer_mix_node_name);
	}
	return true;
}

// 创建 Animator，并绑定宿主节点下的 AnimationPlayer。
static int l_create_animator(lua_State *p_L) {
	if (!_ensure_main_thread("create_animator")) {
		lua_pushinteger(p_L, INVALID_ANIMATOR_ID);
		return 1;
	}

	int32_t owner_node_id = (int32_t)luaL_checkinteger(p_L, 1);
	godot::Node3D *owner = node_resolve(owner_node_id);
	if (owner == nullptr) {
		godot::UtilityFunctions::printerr("native_anim.create_animator: invalid owner node id ", owner_node_id);
		lua_pushinteger(p_L, INVALID_ANIMATOR_ID);
		return 1;
	}

	godot::AnimationPlayer *player = _find_animation_player_recursive(owner);
	if (player == nullptr) {
		godot::UtilityFunctions::printerr("native_anim.create_animator: AnimationPlayer not found under owner");
		lua_pushinteger(p_L, INVALID_ANIMATOR_ID);
		return 1;
	}
	if (!_ensure_empty_animation(player)) {
		lua_pushinteger(p_L, INVALID_ANIMATOR_ID);
		return 1;
	}

	AnimatorRecord animator;
	animator.id = next_animator_id++;
	animator.owner_node_id = owner_node_id;
	animator.animation_player_path = owner->get_path_to(player);
	animator.animation_tree = memnew(godot::AnimationTree);
	animator.animation_tree->set_name(godot::String("_NativeAnimTree") + godot::String::num_int64(animator.id));
	animator.animation_tree->set_callback_mode_process(godot::AnimationMixer::ANIMATION_CALLBACK_MODE_PROCESS_MANUAL);
	animator.animation_tree->set_active(true);
	animator.tree_root.instantiate();

	owner->add_child(animator.animation_tree);
	animator.animation_tree->set_animation_player(animator.animation_tree->get_path_to(player));
	animator.animation_tree->set_tree_root(animator.tree_root);

	godot::Ref<godot::AnimationNodeAnimation> base_node;
	base_node.instantiate();
	base_node->set_animation(EMPTY_ANIM_NAME);
	animator.tree_root->add_node(godot::StringName(BASE_NODE_NAME), base_node);
	animator.tree_root->connect_node(godot::StringName("output"), 0, godot::StringName(BASE_NODE_NAME));

	animators[animator.id] = animator;
	lua_pushinteger(p_L, animator.id);
	return 1;
}

static int l_destroy_animator(lua_State *p_L) {
	if (!_ensure_main_thread("destroy_animator")) {
		return 0;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	if (!animators.has(animator_id)) {
		return 0;
	}
	AnimatorRecord *animator = &animators[animator_id];
	if (animator->animation_tree != nullptr) {
		animator->animation_tree->queue_free();
	}
	animators.erase(animator_id);
	return 0;
}

static int l_is_animator_valid(lua_State *p_L) {
	if (!_ensure_main_thread("is_animator_valid")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	if (!animators.has(animator_id)) {
		lua_pushboolean(p_L, false);
		return 1;
	}
	lua_pushboolean(p_L, _is_animator_runtime_valid(animators[animator_id]));
	return 1;
}

static int l_create_layer(lua_State *p_L) {
	if (!_ensure_main_thread("create_layer")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	int32_t mix_mode = (int32_t)luaL_checkinteger(p_L, 3);
	int32_t order = (int32_t)luaL_checkinteger(p_L, 4);
	int32_t flags = (int32_t)luaL_optinteger(p_L, 5, 0);

	AnimatorRecord *animator = _get_animator(animator_id, "create_layer");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	if (!_is_valid_mix_mode(mix_mode)) {
		godot::UtilityFunctions::printerr("native_anim.create_layer: invalid mix_mode ", mix_mode);
		_push_bool(p_L, false);
		return 1;
	}

	const godot::StringName layer_name(layer_name_cstr);
	if (animator->layers.has(layer_name)) {
		godot::UtilityFunctions::printerr("native_anim.create_layer: duplicated layer: ", godot::String(layer_name));
		_push_bool(p_L, false);
		return 1;
	}

	// Layer 内部固定采用双 slot + layer_mix 模板。
	LayerRecord layer;
	layer.name = layer_name;
	layer.mix_mode = mix_mode;
	layer.order = order;
	layer.flags = flags;
	layer.weight = 1.0;
	layer.speed = 1.0;
	layer.active_slot = SLOT_A;
	layer.current_blend = 0.0;
	layer.target_blend = 0.0;
	layer.fade_duration = 0.0;
	layer.fade_elapsed = 0.0;
	layer.fading = false;
	layer.blend2d_x = 0.0;
	layer.blend2d_y = 0.0;

	layer.slot_a.time_scale_node_name = _make_node_name(layer_name, "slot_a_time");
	layer.slot_b.time_scale_node_name = _make_node_name(layer_name, "slot_b_time");
	layer.slot_switch_node_name = _make_node_name(layer_name, "slot_switch");
	layer.layer_mix_node_name = _make_node_name(layer_name, "layer_mix");
	_slot_reset(&layer.slot_a);
	_slot_reset(&layer.slot_b);

	godot::Ref<godot::AnimationNodeTimeScale> slot_a_time;
	slot_a_time.instantiate();
	animator->tree_root->add_node(layer.slot_a.time_scale_node_name, slot_a_time);

	godot::Ref<godot::AnimationNodeTimeScale> slot_b_time;
	slot_b_time.instantiate();
	animator->tree_root->add_node(layer.slot_b.time_scale_node_name, slot_b_time);

	godot::Ref<godot::AnimationNodeBlend2> slot_switch;
	slot_switch.instantiate();
	animator->tree_root->add_node(layer.slot_switch_node_name, slot_switch);

	if (mix_mode == MIX_BLEND) {
		godot::Ref<godot::AnimationNodeBlend2> layer_mix;
		layer_mix.instantiate();
		animator->tree_root->add_node(layer.layer_mix_node_name, layer_mix);
	} else {
		godot::Ref<godot::AnimationNodeAdd2> layer_mix;
		layer_mix.instantiate();
		animator->tree_root->add_node(layer.layer_mix_node_name, layer_mix);
	}

	animator->tree_root->connect_node(layer.slot_switch_node_name, 0, layer.slot_a.time_scale_node_name);
	animator->tree_root->connect_node(layer.slot_switch_node_name, 1, layer.slot_b.time_scale_node_name);
	animator->tree_root->connect_node(layer.layer_mix_node_name, 1, layer.slot_switch_node_name);

	animator->layers[layer_name] = layer;
	_insert_layer_name(animator, layer_name, order);
	LayerRecord *stored_layer = &animator->layers[layer_name];
	_assign_slot_empty(animator, stored_layer, &stored_layer->slot_a);
	_assign_slot_empty(animator, stored_layer, &stored_layer->slot_b);
	_set_tree_parameter(animator->animation_tree, stored_layer->slot_switch_node_name, "blend_amount", 0.0);
	_set_layer_weight_runtime(animator, stored_layer);
	_rebuild_layer_stack(animator);

	_push_bool(p_L, true);
	return 1;
}

static int l_destroy_layer(lua_State *p_L) {
	if (!_ensure_main_thread("destroy_layer")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	AnimatorRecord *animator = _get_animator(animator_id, "destroy_layer");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	const godot::StringName layer_name(layer_name_cstr);
	LayerRecord *layer = _get_layer(animator, layer_name, "destroy_layer");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	_remove_layer_nodes(animator, layer);
	animator->layers.erase(layer_name);
	_erase_layer_name(animator, layer_name);
	_rebuild_layer_stack(animator);
	_push_bool(p_L, true);
	return 1;
}

static int l_has_layer(lua_State *p_L) {
	if (!_ensure_main_thread("has_layer")) {
		lua_pushboolean(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	AnimatorRecord *animator = _get_animator(animator_id, "has_layer");
	if (animator == nullptr) {
		lua_pushboolean(p_L, false);
		return 1;
	}
	lua_pushboolean(p_L, animator->layers.has(godot::StringName(layer_name_cstr)));
	return 1;
}

// 播放普通动画；anim_name 传空字符串时等价于停止该 Layer。
static int l_play(lua_State *p_L) {
	if (!_ensure_main_thread("play")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	const char *anim_name_cstr = luaL_checkstring(p_L, 3);
	double fade_time = luaL_optnumber(p_L, 4, 0.0);

	AnimatorRecord *animator = _get_animator(animator_id, "play");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	LayerRecord *layer = _get_layer(animator, godot::StringName(layer_name_cstr), "play");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t target_slot_index = layer->active_slot == SLOT_A ? SLOT_B : SLOT_A;
	SlotRecord *target_slot = _layer_slot(layer, target_slot_index);
	if (anim_name_cstr[0] == '\0') {
		if (!_assign_slot_empty(animator, layer, target_slot)) {
			_push_bool(p_L, false);
			return 1;
		}
	} else if (!_assign_slot_anim(animator, layer, target_slot, godot::StringName(anim_name_cstr))) {
		_push_bool(p_L, false);
		return 1;
	}
	_start_layer_fade(animator, layer, target_slot_index, fade_time);
	_push_bool(p_L, true);
	return 1;
}

static int l_clear_blend2d(lua_State *p_L) {
	if (!_ensure_main_thread("clear_blend2d")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	AnimatorRecord *animator = _get_animator(animator_id, "clear_blend2d");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	LayerRecord *layer = _get_layer(animator, godot::StringName(layer_name_cstr), "clear_blend2d");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	layer->blend2d_points.clear();
	_push_bool(p_L, true);
	return 1;
}

static int l_set_blend2d_point(lua_State *p_L) {
	if (!_ensure_main_thread("set_blend2d_point")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	const char *anim_name_cstr = luaL_checkstring(p_L, 3);
	double x = luaL_checknumber(p_L, 4);
	double y = luaL_checknumber(p_L, 5);

	AnimatorRecord *animator = _get_animator(animator_id, "set_blend2d_point");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	LayerRecord *layer = _get_layer(animator, godot::StringName(layer_name_cstr), "set_blend2d_point");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	const godot::StringName anim_name(anim_name_cstr);
	for (int32_t i = 0; i < layer->blend2d_points.size(); i++) {
		if (layer->blend2d_points[i].anim_name == anim_name) {
			layer->blend2d_points.write[i].position = godot::Vector2((float)x, (float)y);
			_push_bool(p_L, true);
			return 1;
		}
	}
	Blend2DPointRecord point;
	point.anim_name = anim_name;
	point.position = godot::Vector2((float)x, (float)y);
	layer->blend2d_points.push_back(point);
	_push_bool(p_L, true);
	return 1;
}

static int l_play_blend2d(lua_State *p_L) {
	if (!_ensure_main_thread("play_blend2d")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	double fade_time = luaL_optnumber(p_L, 3, 0.0);

	AnimatorRecord *animator = _get_animator(animator_id, "play_blend2d");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	LayerRecord *layer = _get_layer(animator, godot::StringName(layer_name_cstr), "play_blend2d");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t target_slot_index = layer->active_slot == SLOT_A ? SLOT_B : SLOT_A;
	SlotRecord *target_slot = _layer_slot(layer, target_slot_index);
	if (!_assign_slot_blend2d(animator, layer, target_slot)) {
		_push_bool(p_L, false);
		return 1;
	}
	_start_layer_fade(animator, layer, target_slot_index, fade_time);
	_push_bool(p_L, true);
	return 1;
}

static int l_set_blend2d_params(lua_State *p_L) {
	if (!_ensure_main_thread("set_blend2d_params")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	double x = luaL_checknumber(p_L, 3);
	double y = luaL_checknumber(p_L, 4);

	AnimatorRecord *animator = _get_animator(animator_id, "set_blend2d_params");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	LayerRecord *layer = _get_layer(animator, godot::StringName(layer_name_cstr), "set_blend2d_params");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	layer->blend2d_x = x;
	layer->blend2d_y = y;
	_set_slot_blend_position_runtime(animator, layer, &layer->slot_a);
	_set_slot_blend_position_runtime(animator, layer, &layer->slot_b);
	_push_bool(p_L, true);
	return 1;
}

static int l_set_layer_weight(lua_State *p_L) {
	if (!_ensure_main_thread("set_layer_weight")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	double weight = luaL_checknumber(p_L, 3);
	AnimatorRecord *animator = _get_animator(animator_id, "set_layer_weight");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	LayerRecord *layer = _get_layer(animator, godot::StringName(layer_name_cstr), "set_layer_weight");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	layer->weight = weight;
	_set_layer_weight_runtime(animator, layer);
	_push_bool(p_L, true);
	return 1;
}

static int l_set_layer_speed(lua_State *p_L) {
	if (!_ensure_main_thread("set_layer_speed")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	const char *layer_name_cstr = luaL_checkstring(p_L, 2);
	double speed = luaL_checknumber(p_L, 3);
	AnimatorRecord *animator = _get_animator(animator_id, "set_layer_speed");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	LayerRecord *layer = _get_layer(animator, godot::StringName(layer_name_cstr), "set_layer_speed");
	if (layer == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}
	layer->speed = speed;
	_set_slot_speed_runtime(animator, layer, &layer->slot_a);
	_set_slot_speed_runtime(animator, layer, &layer->slot_b);
	_push_bool(p_L, true);
	return 1;
}

// 由 Lua 显式推进 fade 和 AnimationTree。
static int l_update(lua_State *p_L) {
	if (!_ensure_main_thread("update")) {
		_push_bool(p_L, false);
		return 1;
	}

	int32_t animator_id = (int32_t)luaL_checkinteger(p_L, 1);
	double delta = luaL_checknumber(p_L, 2);
	AnimatorRecord *animator = _get_animator(animator_id, "update");
	if (animator == nullptr) {
		_push_bool(p_L, false);
		return 1;
	}

	for (int32_t i = 0; i < animator->layer_order.size(); i++) {
		const godot::StringName &layer_name = animator->layer_order[i];
		if (!animator->layers.has(layer_name)) {
			continue;
		}
		LayerRecord *layer = &animator->layers[layer_name];
		if (layer->fading) {
			layer->fade_elapsed += delta;
			double t = layer->fade_duration <= 0.0 ? 1.0 : (layer->fade_elapsed / layer->fade_duration);
			if (t >= 1.0) {
				t = 1.0;
				layer->fading = false;
			}
			layer->current_blend = layer->current_blend + (layer->target_blend - layer->current_blend) * t;
			if (!layer->fading) {
				layer->current_blend = layer->target_blend;
			}
			_set_tree_parameter(animator->animation_tree, layer->slot_switch_node_name, "blend_amount", layer->current_blend);
		}
	}

	animator->animation_tree->advance((float)delta);
	_push_bool(p_L, true);
	return 1;
}

static const luaL_Reg anim_funcs[] = {
	{"create_animator", l_create_animator},
	{"destroy_animator", l_destroy_animator},
	{"is_animator_valid", l_is_animator_valid},
	{"create_layer", l_create_layer},
	{"destroy_layer", l_destroy_layer},
	{"has_layer", l_has_layer},
	{"play", l_play},
	{"clear_blend2d", l_clear_blend2d},
	{"set_blend2d_point", l_set_blend2d_point},
	{"play_blend2d", l_play_blend2d},
	{"set_blend2d_params", l_set_blend2d_params},
	{"set_layer_weight", l_set_layer_weight},
	{"set_layer_speed", l_set_layer_speed},
	{"update", l_update},
	{nullptr, nullptr}
};

int luaopen_native_anim(lua_State *p_L) {
	luaL_newlib(p_L, anim_funcs);
	lua_pushinteger(p_L, MIX_BLEND);
	lua_setfield(p_L, -2, "MIX_BLEND");
	lua_pushinteger(p_L, MIX_ADD);
	lua_setfield(p_L, -2, "MIX_ADD");
	lua_pushinteger(p_L, FLAG_NONE);
	lua_setfield(p_L, -2, "FLAG_NONE");
	lua_pushinteger(p_L, FLAG_ALLOW_BLEND2D);
	lua_setfield(p_L, -2, "FLAG_ALLOW_BLEND2D");
	return 1;
}

void anim_cleanup() {
	for (const godot::KeyValue<int32_t, AnimatorRecord> &kv : animators) {
		const AnimatorRecord &animator = kv.value;
		if (animator.animation_tree != nullptr) {
			animator.animation_tree->queue_free();
		}
	}
	animators.clear();
	next_animator_id = 1;
}

} // namespace luagd
