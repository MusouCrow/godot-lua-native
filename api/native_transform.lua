---@meta

---@class native_transform
local M = {}

--- native_transform.set_position(id, x, y, z, is_global) -> void
--- 设置节点位置。
---@param id integer 节点句柄
---@param x number X 坐标
---@param y number Y 坐标
---@param z number Z 坐标
---@param is_global? boolean true 为世界坐标，false 为局部坐标
function M.set_position(id, x, y, z, is_global) end

--- native_transform.get_position(id, is_global) -> number, number, number
--- 获取节点位置。
---@param id integer 节点句柄
---@param is_global? boolean true 为世界坐标，false 为局部坐标
---@return number x
---@return number y
---@return number z
function M.get_position(id, is_global) end

--- native_transform.get_scale(id, is_global) -> number, number, number
--- 获取节点缩放。
---@param id integer 节点句柄
---@param is_global? boolean true 为世界缩放，false 为局部缩放
---@return number x
---@return number y
---@return number z
function M.get_scale(id, is_global) end

--- native_transform.set_rotation(id, x, y, z, is_global) -> void
--- 设置节点旋转（度数）。
---@param id integer 节点句柄
---@param x number X 轴旋转
---@param y number Y 轴旋转
---@param z number Z 轴旋转
---@param is_global? boolean true 为世界旋转，false 为局部旋转
function M.set_rotation(id, x, y, z, is_global) end

--- native_transform.get_rotation(id, is_global) -> number, number, number
--- 获取节点旋转（度数）。
---@param id integer 节点句柄
---@param is_global? boolean true 为世界旋转，false 为局部旋转
---@return number x
---@return number y
---@return number z
function M.get_rotation(id, is_global) end

--- native_transform.look_at(id, target_x, target_y, target_z, use_model_front) -> void
--- 使节点朝向目标位置。
---@param id integer 节点句柄
---@param target_x number 目标 X 坐标
---@param target_y number 目标 Y 坐标
---@param target_z number 目标 Z 坐标
---@param use_model_front? boolean true 时 +Z 指向目标，false 时 -Z 指向目标
function M.look_at(id, target_x, target_y, target_z, use_model_front) end

--- native_transform.get_forward(id, is_global, use_model_front) -> number, number, number
--- 获取节点前向向量。
---@param id integer 节点句柄
---@param is_global? boolean true 为世界空间前向，false 为局部前向
---@param use_model_front? boolean true 返回 +Z，false 返回 -Z
---@return number x
---@return number y
---@return number z
function M.get_forward(id, is_global, use_model_front) end

return M
