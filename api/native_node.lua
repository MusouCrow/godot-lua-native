---@meta

---@class native_node
local M = {}

-- ============================================================================
-- 节点引用管理
-- ============================================================================

--- native_node.get_by_path(path) -> int
--- 通过场景路径获取节点句柄。
--- 注意：目前仅支持 CharacterBody3D 节点。
---@param path string 节点路径（如 "%Player" 或 "../Player"）
---@return integer id 节点句柄，失败返回 -1
function M.get_by_path(path) end

--- native_node.release(id) -> void
--- 释放节点引用。
--- 注意：不会销毁节点本身，仅释放模块内的引用。
---@param id integer 节点句柄
function M.release(id) end

--- native_node.is_valid(id) -> boolean
--- 检查节点引用是否有效。
---@param id integer 节点句柄
---@return boolean valid 是否有效
function M.is_valid(id) end

-- ============================================================================
-- 位置
-- ============================================================================

--- native_node.set_position(id, x, y, z, is_global) -> void
--- 设置节点位置。
---@param id integer 节点句柄
---@param x number X 坐标
---@param y number Y 坐标
---@param z number Z 坐标
---@param is_global? boolean true 为世界坐标，false 为局部坐标（默认 false）
function M.set_position(id, x, y, z, is_global) end

--- native_node.get_position(id, is_global) -> number, number, number
--- 获取节点位置。
---@param id integer 节点句柄
---@param is_global? boolean true 为世界坐标，false 为局部坐标（默认 false）
---@return number x X 坐标
---@return number y Y 坐标
---@return number z Z 坐标
function M.get_position(id, is_global) end

-- ============================================================================
-- 旋转
-- ============================================================================

--- native_node.set_rotation(id, x, y, z, is_global) -> void
--- 设置节点旋转（弧度）。
---@param id integer 节点句柄
---@param x number X 轴旋转（弧度）
---@param y number Y 轴旋转（弧度）
---@param z number Z 轴旋转（弧度）
---@param is_global? boolean true 为世界旋转，false 为局部旋转（默认 false）
function M.set_rotation(id, x, y, z, is_global) end

--- native_node.get_rotation(id, is_global) -> number, number, number
--- 获取节点旋转（弧度）。
---@param id integer 节点句柄
---@param is_global? boolean true 为世界旋转，false 为局部旋转（默认 false）
---@return number x X 轴旋转（弧度）
---@return number y Y 轴旋转（弧度）
---@return number z Z 轴旋转（弧度）
function M.get_rotation(id, is_global) end

--- native_node.look_at(id, target_x, target_y, target_z) -> void
--- 使节点朝向目标位置。
--- 节点的 -Z 轴（前向）将指向目标位置。
---@param id integer 节点句柄
---@param target_x number 目标 X 坐标
---@param target_y number 目标 Y 坐标
---@param target_z number 目标 Z 坐标
function M.look_at(id, target_x, target_y, target_z) end

-- ============================================================================
-- 移动
-- ============================================================================

--- native_node.move_and_slide(id) -> boolean
--- 执行移动并滑动。
--- 应在 physics_process 中调用。
--- 修改节点的 velocity 属性。
---@param id integer 节点句柄
---@return boolean collided 是否发生碰撞
function M.move_and_slide(id) end

--- native_node.set_velocity(id, x, y, z) -> void
--- 设置节点的速度向量。
--- 注意：不要乘以 delta，move_and_slide 会自动处理。
---@param id integer 节点句柄
---@param x number X 方向速度
---@param y number Y 方向速度
---@param z number Z 方向速度
function M.set_velocity(id, x, y, z) end

--- native_node.get_velocity(id) -> number, number, number
--- 获取节点的速度向量。
---@param id integer 节点句柄
---@return number x X 方向速度
---@return number y Y 方向速度
---@return number z Z 方向速度
function M.get_velocity(id) end

--- native_node.get_real_velocity(id) -> number, number, number
--- 获取节点的实际移动速度。
--- 考虑滑动后的实际速度。
---@param id integer 节点句柄
---@return number x X 方向实际速度
---@return number y Y 方向实际速度
---@return number z Z 方向实际速度
function M.get_real_velocity(id) end

--- native_node.is_on_floor(id) -> boolean
--- 检查节点是否在地面上。
--- 仅在 move_and_slide 调用后有效。
---@param id integer 节点句柄
---@return boolean on_floor 是否在地面
function M.is_on_floor(id) end

--- native_node.is_on_wall(id) -> boolean
--- 检查节点是否在墙上。
--- 仅在 move_and_slide 调用后有效。
---@param id integer 节点句柄
---@return boolean on_wall 是否在墙
function M.is_on_wall(id) end

--- native_node.is_on_ceiling(id) -> boolean
--- 检查节点是否在天花板上。
--- 仅在 move_and_slide 调用后有效。
---@param id integer 节点句柄
---@return boolean on_ceiling 是否在天花板
function M.is_on_ceiling(id) end

--- native_node.get_floor_normal(id) -> number, number, number
--- 获取地面的碰撞法线。
--- 仅在 move_and_slide 调用后且 is_on_floor 为 true 时有效。
---@param id integer 节点句柄
---@return number nx 法线 X 分量
---@return number ny 法线 Y 分量
---@return number nz 法线 Z 分量
function M.get_floor_normal(id) end

-- ============================================================================
-- 信息
-- ============================================================================

--- native_node.get_name(id) -> string
--- 获取节点名称。
---@param id integer 节点句柄
---@return string name 节点名称
function M.get_name(id) end

return M
