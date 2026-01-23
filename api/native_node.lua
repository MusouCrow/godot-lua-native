---@meta

---@class native_node
local M = {}

-- ============================================================================
-- 节点引用管理
-- ============================================================================

--- native_node.set_root(path) -> boolean
--- 设置根节点路径，后续创建的节点将挂载到此节点下。
---@param path string 根节点路径（如 "/root/pre_entry/pre_scene/root"）
---@return boolean success 是否设置成功
function M.set_root(path) end

--- native_node.instantiate(scene_path) -> int
--- 从场景资源路径加载并实例化节点，挂载到根节点下。
--- 注意：必须先调用 set_root 设置根节点。
---@param scene_path string 场景资源路径（如 "res://assets/char/sm_char_proto.glb"）
---@return integer id 节点句柄，失败返回 -1
function M.instantiate(scene_path) end

--- native_node.destroy(id) -> void
--- 销毁节点并释放引用。
--- 对于通过 create_from_scene 创建的节点，调用 queue_free 销毁节点。
--- 对于通过 get_by_path 获取的节点，仅释放引用。
---@param id integer 节点句柄
function M.destroy(id) end

--- native_node.get_by_path(path) -> int
--- [已废弃] 通过场景路径获取节点句柄。
--- 请使用 create_from_scene 替代。
---@deprecated 使用 create_from_scene 替代
---@param path string 节点路径（如 "/root/pre_entry/pre_scene/root/sm_char_proto"）
---@return integer id 节点句柄，失败返回 -1
function M.get_by_path(path) end

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

--- native_node.look_at(id, target_x, target_y, target_z, use_model_front) -> void
--- 使节点朝向目标位置。
---@param id integer 节点句柄
---@param target_x number 目标 X 坐标
---@param target_y number 目标 Y 坐标
---@param target_z number 目标 Z 坐标
---@param use_model_front? boolean true 时 +Z 轴指向目标（模型前向），false 时 -Z 轴指向目标（默认 false）
function M.look_at(id, target_x, target_y, target_z, use_model_front) end

--- native_node.get_forward(id, is_global, use_model_front) -> number, number, number
--- 获取节点的前向向量（归一化）。
---@param id integer 节点句柄
---@param is_global? boolean true 为世界空间前向，false 为局部前向（默认 false）
---@param use_model_front? boolean true 返回 +Z 轴（模型前向），false 返回 -Z 轴（默认 false）
---@return number x 前向向量 X 分量
---@return number y 前向向量 Y 分量
---@return number z 前向向量 Z 分量
function M.get_forward(id, is_global, use_model_front) end

-- ============================================================================
-- 移动
-- ============================================================================

--- native_node.move_and_slide(id) -> boolean
--- 执行移动并滑动。
--- 应在 physics_process 中调用。
--- 仅支持 CharacterBody3D 节点。
---@param id integer 节点句柄
---@return boolean collided 是否发生碰撞
function M.move_and_slide(id) end

--- native_node.set_velocity(id, x, y, z) -> void
--- 设置节点的速度向量。
--- 注意：不要乘以 delta，move_and_slide 会自动处理。
--- 仅支持 CharacterBody3D 节点。
---@param id integer 节点句柄
---@param x number X 方向速度
---@param y number Y 方向速度
---@param z number Z 方向速度
function M.set_velocity(id, x, y, z) end

--- native_node.get_velocity(id) -> number, number, number
--- 获取节点的速度向量。
--- 仅支持 CharacterBody3D 节点。
---@param id integer 节点句柄
---@return number x X 方向速度
---@return number y Y 方向速度
---@return number z Z 方向速度
function M.get_velocity(id) end

--- native_node.get_real_velocity(id) -> number, number, number
--- 获取节点的实际移动速度。
--- 考虑滑动后的实际速度。
--- 仅支持 CharacterBody3D 节点。
---@param id integer 节点句柄
---@return number x X 方向实际速度
---@return number y Y 方向实际速度
---@return number z Z 方向实际速度
function M.get_real_velocity(id) end

--- native_node.is_on_floor(id) -> boolean
--- 检查节点是否在地面上。
--- 仅在 move_and_slide 调用后有效。
--- 仅支持 CharacterBody3D 节点。
---@param id integer 节点句柄
---@return boolean on_floor 是否在地面
function M.is_on_floor(id) end

--- native_node.is_on_wall(id) -> boolean
--- 检查节点是否在墙上。
--- 仅在 move_and_slide 调用后有效。
--- 仅支持 CharacterBody3D 节点。
---@param id integer 节点句柄
---@return boolean on_wall 是否在墙
function M.is_on_wall(id) end

--- native_node.is_on_ceiling(id) -> boolean
--- 检查节点是否在天花板上。
--- 仅在 move_and_slide 调用后有效。
--- 仅支持 CharacterBody3D 节点。
---@param id integer 节点句柄
---@return boolean on_ceiling 是否在天花板
function M.is_on_ceiling(id) end

--- native_node.get_floor_normal(id) -> number, number, number
--- 获取地面的碰撞法线。
--- 仅在 move_and_slide 调用后且 is_on_floor 为 true 时有效。
--- 仅支持 CharacterBody3D 节点。
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

--- native_node.get_type(id) -> string
--- 获取节点类型。
---@param id integer 节点句柄
---@return string type 节点类型（"Node3D" 或 "CharacterBody3D"）
function M.get_type(id) end

return M
