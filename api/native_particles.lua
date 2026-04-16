---@meta

---@class native_particles
local M = {}

--- native_particles.play(node_id) -> bool
--- 开始发射粒子。
---@param node_id integer native_node 返回的粒子节点 id
---@return boolean success 是否成功
function M.play(node_id) end

--- native_particles.stop(node_id) -> bool
--- 停止继续发射新粒子，但不清空现有粒子。
---@param node_id integer native_node 返回的粒子节点 id
---@return boolean success 是否成功
function M.stop(node_id) end

--- native_particles.clear(node_id) -> bool
--- 清空现有粒子，同时保持调用前的播放状态不变。
---@param node_id integer native_node 返回的粒子节点 id
---@return boolean success 是否成功
function M.clear(node_id) end

--- native_particles.update(node_id, delta) -> bool
--- 请求粒子在单帧内额外处理一段时间。
--- 注意：这不是像 native_anim.update() 那样的主时间推进。
---@param node_id integer native_node 返回的粒子节点 id
---@param delta number 单帧额外处理时间，要求 >= 0
---@return boolean success 是否成功
function M.update(node_id, delta) end

--- native_particles.set_speed_scale(node_id, speed_scale) -> bool
--- 设置粒子模拟速度倍率；传 0 可用于冻结模拟。
---@param node_id integer native_node 返回的粒子节点 id
---@param speed_scale number 模拟速度倍率
---@return boolean success 是否成功
function M.set_speed_scale(node_id, speed_scale) end

--- native_particles.is_playing(node_id) -> bool
--- 查询当前是否仍在发射新粒子。
---@param node_id integer native_node 返回的粒子节点 id
---@return boolean playing 是否处于发射状态
function M.is_playing(node_id) end

--- native_particles.is_alive(node_id) -> bool
--- 查询粒子系统是否仍处于活跃状态。
--- 这不是精确的粒子数量统计；对于非 one_shot，可用于 stop 后判断尾巴是否基本结束。
--- 注意：speed_scale 为 0 等暂停场景下，可能长时间保持 alive。
--- 注意：该接口依赖真实粒子渲染后端维护 inactive 状态；在默认 headless/dummy 后端下不保证结果可靠。
---@param node_id integer native_node 返回的粒子节点 id
---@return boolean alive 是否仍未进入 inactive 状态
function M.is_alive(node_id) end

return M
