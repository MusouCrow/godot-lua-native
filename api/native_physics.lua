---@meta

---@class native_physics
local M = {}

--- native_physics.move_and_slide(id) -> boolean
--- 执行 CharacterBody3D 的移动并滑动。
---@param id integer 节点句柄
---@return boolean collided
function M.move_and_slide(id) end

--- native_physics.move_and_collide(id, x, y, z, test_only, safe_margin, recovery_as_collision, max_collisions)
--- 按位移移动 PhysicsBody3D，并返回首个碰撞信息。x/y/z 是本帧位移，不是速度。
---@param id integer 节点句柄
---@param x number
---@param y number
---@param z number
---@param test_only? boolean
---@param safe_margin? number
---@param recovery_as_collision? boolean
---@param max_collisions? integer
---@return boolean collided
---@return number travel_x
---@return number travel_y
---@return number travel_z
---@return number remainder_x
---@return number remainder_y
---@return number remainder_z
---@return number normal_x
---@return number normal_y
---@return number normal_z
---@return number position_x
---@return number position_y
---@return number position_z
---@return integer collider_id
function M.move_and_collide(id, x, y, z, test_only, safe_margin, recovery_as_collision, max_collisions) end

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

--- native_physics.set_collision_layer(id, layer) -> void
--- 设置 CollisionObject3D 的碰撞层（32位掩码）。
---@param id integer 节点句柄
---@param layer integer 碰撞层值（0-4294967295）
function M.set_collision_layer(id, layer) end

--- native_physics.get_collision_layer(id) -> integer
--- 获取 CollisionObject3D 的碰撞层。
---@param id integer 节点句柄
---@return integer layer 碰撞层值，失败返回 0
function M.get_collision_layer(id) end

--- native_physics.set_collision_mask(id, mask) -> void
--- 设置 CollisionObject3D 的碰撞掩码（32位掩码）。
---@param id integer 节点句柄
---@param mask integer 碰撞掩码值（0-4294967295）
function M.set_collision_mask(id, mask) end

--- native_physics.get_collision_mask(id) -> integer
--- 获取 CollisionObject3D 的碰撞掩码。
---@param id integer 节点句柄
---@return integer mask 碰撞掩码值，失败返回 0
function M.get_collision_mask(id) end

return M
