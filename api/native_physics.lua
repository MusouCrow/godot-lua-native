---@meta

---@class native_physics
local M = {}

--- native_physics.get_aabb(id) -> number, number, number, number, number, number
--- 获取主碰撞体在节点自身坐标系下的 AABB。
---@param id integer 节点句柄
---@return number pos_x
---@return number pos_y
---@return number pos_z
---@return number size_x
---@return number size_y
---@return number size_z
function M.get_aabb(id) end

--- native_physics.move_and_slide(id) -> boolean
--- 执行 CharacterBody3D 的移动并滑动。
---@param id integer 节点句柄
---@return boolean collided
function M.move_and_slide(id) end

--- native_physics.set_velocity(id, x, y, z) -> void
--- 设置 CharacterBody3D 的速度。
---@param id integer 节点句柄
---@param x number
---@param y number
---@param z number
function M.set_velocity(id, x, y, z) end

--- native_physics.get_velocity(id) -> number, number, number
--- 获取 CharacterBody3D 的速度。
---@param id integer 节点句柄
---@return number x
---@return number y
---@return number z
function M.get_velocity(id) end

--- native_physics.get_real_velocity(id) -> number, number, number
--- 获取 CharacterBody3D 的实际移动速度。
---@param id integer 节点句柄
---@return number x
---@return number y
---@return number z
function M.get_real_velocity(id) end

--- native_physics.is_on_floor(id) -> boolean
--- 检查 CharacterBody3D 是否在地面上。
---@param id integer 节点句柄
---@return boolean on_floor
function M.is_on_floor(id) end

--- native_physics.is_on_wall(id) -> boolean
--- 检查 CharacterBody3D 是否在墙上。
---@param id integer 节点句柄
---@return boolean on_wall
function M.is_on_wall(id) end

--- native_physics.is_on_ceiling(id) -> boolean
--- 检查 CharacterBody3D 是否在天花板上。
---@param id integer 节点句柄
---@return boolean on_ceiling
function M.is_on_ceiling(id) end

--- native_physics.get_floor_normal(id) -> number, number, number
--- 获取 CharacterBody3D 当前地面法线。
---@param id integer 节点句柄
---@return number nx
---@return number ny
---@return number nz
function M.get_floor_normal(id) end

return M
