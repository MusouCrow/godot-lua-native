---@meta

---@class native_anim
local M = {}

M.MIX_BLEND = 0
M.MIX_ADD = 1

M.FLAG_NONE = 0
M.FLAG_ALLOW_BLEND2D = 1

--- native_anim.create_animator(owner_node_id) -> int
--- 创建 Animator，自动绑定 owner 节点下的 AnimationPlayer。
---@param owner_node_id integer native_node 返回的节点 id
---@return integer animator_id Animator id，失败返回 -1
function M.create_animator(owner_node_id) end

--- native_anim.destroy_animator(animator_id) -> void
---@param animator_id integer Animator id
function M.destroy_animator(animator_id) end

--- native_anim.is_animator_valid(animator_id) -> bool
---@param animator_id integer Animator id
---@return boolean valid 是否有效
function M.is_animator_valid(animator_id) end

--- native_anim.create_layer(animator_id, layer_name, mix_mode, order, flags) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@param mix_mode integer 混合模式，使用 MIX_* 常量
---@param order integer Layer 顺序
---@param flags? integer Layer 标记，使用 FLAG_* 常量
---@return boolean success 是否成功
function M.create_layer(animator_id, layer_name, mix_mode, order, flags) end

--- native_anim.destroy_layer(animator_id, layer_name) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@return boolean success 是否成功
function M.destroy_layer(animator_id, layer_name) end

--- native_anim.has_layer(animator_id, layer_name) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@return boolean has_layer 是否存在
function M.has_layer(animator_id, layer_name) end

--- native_anim.play(animator_id, layer_name, anim_name, fade_time) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@param anim_name string 动画名；传空字符串时等价于停止该 Layer
---@param fade_time? number 淡入时间
---@return boolean success 是否成功
function M.play(animator_id, layer_name, anim_name, fade_time) end

--- native_anim.clear_blend2d(animator_id, layer_name) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@return boolean success 是否成功
function M.clear_blend2d(animator_id, layer_name) end

--- native_anim.set_blend2d_point(animator_id, layer_name, anim_name, x, y) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@param anim_name string 动画名
---@param x number Blend2D X 坐标
---@param y number Blend2D Y 坐标
---@return boolean success 是否成功
function M.set_blend2d_point(animator_id, layer_name, anim_name, x, y) end

--- native_anim.play_blend2d(animator_id, layer_name, fade_time) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@param fade_time? number 淡入时间
---@return boolean success 是否成功
function M.play_blend2d(animator_id, layer_name, fade_time) end

--- native_anim.set_blend2d_params(animator_id, layer_name, x, y) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@param x number Blend2D X 输入
---@param y number Blend2D Y 输入
---@return boolean success 是否成功
function M.set_blend2d_params(animator_id, layer_name, x, y) end

--- native_anim.set_layer_weight(animator_id, layer_name, weight) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@param weight number Layer 权重
---@return boolean success 是否成功
function M.set_layer_weight(animator_id, layer_name, weight) end

--- native_anim.set_layer_speed(animator_id, layer_name, speed) -> bool
---@param animator_id integer Animator id
---@param layer_name string Layer 名称
---@param speed number Layer 速度
---@return boolean success 是否成功
function M.set_layer_speed(animator_id, layer_name, speed) end

--- native_anim.update(animator_id, delta) -> bool
---@param animator_id integer Animator id
---@param delta number 帧推进时间
---@return boolean success 是否成功
function M.update(animator_id, delta) end

return M
