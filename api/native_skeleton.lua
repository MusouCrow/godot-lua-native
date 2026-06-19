---@meta

---@class native_skeleton
local M = {}

--- native_skeleton.bone_exists(skeleton_node_id, bone_name) -> bool
--- 检查骨骼是否存在。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_name string 骨骼名称
---@return boolean exists 骨骼是否存在
function M.bone_exists(skeleton_node_id, bone_name) end

--- native_skeleton.get_bone_count(skeleton_node_id) -> int
--- 获取骨骼数量。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@return integer count 骨骼数量
function M.get_bone_count(skeleton_node_id) end

--- native_skeleton.get_bone_name(skeleton_node_id, bone_idx) -> string
--- 按索引获取骨骼名称。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_idx integer 骨骼索引
---@return string name 骨骼名称
function M.get_bone_name(skeleton_node_id, bone_idx) end

--- native_skeleton.set_bone_position(skeleton_node_id, bone_name, x, y, z, is_global) -> void
--- 设置骨骼位置。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_name string 骨骼名称
---@param x number X 坐标
---@param y number Y 坐标
---@param z number Z 坐标
---@param is_global? boolean true=世界坐标，false=局部坐标
function M.set_bone_position(skeleton_node_id, bone_name, x, y, z, is_global) end

--- native_skeleton.get_bone_position(skeleton_node_id, bone_name, is_global) -> number, number, number
--- 获取骨骼位置。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_name string 骨骼名称
---@param is_global? boolean true=世界坐标，false=局部坐标
---@return number x
---@return number y
---@return number z
function M.get_bone_position(skeleton_node_id, bone_name, is_global) end

--- native_skeleton.set_bone_rotation(skeleton_node_id, bone_name, x, y, z, is_global) -> void
--- 设置骨骼旋转（度数）。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_name string 骨骼名称
---@param x number X 轴旋转（度数）
---@param y number Y 轴旋转（度数）
---@param z number Z 轴旋转（度数）
---@param is_global? boolean true=世界旋转，false=局部旋转
function M.set_bone_rotation(skeleton_node_id, bone_name, x, y, z, is_global) end

--- native_skeleton.get_bone_rotation(skeleton_node_id, bone_name, is_global) -> number, number, number
--- 获取骨骼旋转（度数）。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_name string 骨骼名称
---@param is_global? boolean true=世界旋转，false=局部旋转
---@return number x
---@return number y
---@return number z
function M.get_bone_rotation(skeleton_node_id, bone_name, is_global) end

--- native_skeleton.set_bone_scale(skeleton_node_id, bone_name, x, y, z, is_global) -> void
--- 设置骨骼缩放。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_name string 骨骼名称
---@param x number X 缩放
---@param y number Y 缩放
---@param z number Z 缩放
---@param is_global? boolean true=世界缩放，false=局部缩放
function M.set_bone_scale(skeleton_node_id, bone_name, x, y, z, is_global) end

--- native_skeleton.get_bone_scale(skeleton_node_id, bone_name, is_global) -> number, number, number
--- 获取骨骼缩放。
---@param skeleton_node_id integer Skeleton3D 节点句柄
---@param bone_name string 骨骼名称
---@param is_global? boolean true=世界缩放，false=局部缩放
---@return number x
---@return number y
---@return number z
function M.get_bone_scale(skeleton_node_id, bone_name, is_global) end

return M
