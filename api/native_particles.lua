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

--- native_particles.emit_particle(node_id, pos_x, pos_y, pos_z, vel_x?, vel_y?, vel_z?, r?, g?, b?, a?, custom_x?, custom_y?, custom_z?, custom_w?, rot_x?, rot_y?, rot_z?, scale_x?, scale_y?, scale_z?) -> bool
--- 强制发射单个粒子。
--- 位置必填；速度、颜色、custom、旋转(度数)、缩放均可省略，省略项不施加覆盖，由粒子材质自行随机取值。
--- 注意：默认 ParticleProcessMaterial 下 custom 语义为 (rotation, age, animation, lifetime)。
--- 注意：仅 Forward+ 与 Mobile 渲染方法支持该接口；调用后会关闭持续发射，需配合 play 重新开启。
---@param node_id integer native_node 返回的粒子节点 id
---@param pos_x number 粒子位置 X
---@param pos_y number 粒子位置 Y
---@param pos_z number 粒子位置 Z
---@param vel_x number|nil 速度 X
---@param vel_y number|nil 速度 Y
---@param vel_z number|nil 速度 Z
---@param r number|nil 颜色 R
---@param g number|nil 颜色 G
---@param b number|nil 颜色 B
---@param a number|nil 颜色 A
---@param custom_x number|nil custom X
---@param custom_y number|nil custom Y
---@param custom_z number|nil custom Z
---@param custom_w number|nil custom W
---@param rot_x number|nil 旋转 X（度数）
---@param rot_y number|nil 旋转 Y（度数）
---@param rot_z number|nil 旋转 Z（度数）
---@param scale_x number|nil 缩放 X
---@param scale_y number|nil 缩放 Y
---@param scale_z number|nil 缩放 Z
---@return boolean success 是否成功
function M.emit_particle(node_id, pos_x, pos_y, pos_z, vel_x, vel_y, vel_z, r, g, b, a, custom_x, custom_y, custom_z, custom_w, rot_x, rot_y, rot_z, scale_x, scale_y, scale_z) end

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
