---@meta

---@class native_collision
local M = {}

--- native_collision.get_aabb(id) -> number, number, number, number, number, number
--- 获取主碰撞体在节点自身坐标系下的 AABB。
---@param id integer 节点句柄
---@return number pos_x
---@return number pos_y
---@return number pos_z
---@return number size_x
---@return number size_y
---@return number size_z
function M.get_aabb(id) end

--- native_collision.intersect_hitbox(node_id, collision_mask, callback) -> void
--- 对指定节点或其子节点中的 AttackHitbox3D 执行碰撞检测。
--- 如果 node_id 本身是 AttackHitbox3D，直接处理；
--- 否则遍历其直接子节点（仅一层），找到所有 AttackHitbox3D 进行处理。
--- 对每个碰撞目标调用 callback 函数，传入 target_id（ObjectID）。
--- 多个 hitbox 检测到同一目标时，只回调一次（自动去重）。
--- callback 返回 false 可提前终止迭代。
---@param node_id integer 节点的 ObjectID（可以是 AttackHitbox3D 或包含 AttackHitbox3D 子节点的父节点）
---@param collision_mask integer 碰撞层掩码，0 表示检测所有层（0xFFFFFFFF）
---@param callback fun(target_id: integer): boolean 回调函数，参数为碰撞目标的 ObjectID，返回 false 终止迭代
function M.intersect_hitbox(node_id, collision_mask, callback) end

--- native_collision.intersect_cylinder(ref_node_id, pos_x, pos_y, pos_z, forward_x, forward_y, forward_z, radius, height, angle, collision_mask, callback) -> void
--- 对指定位置执行圆柱/扇柱空间检测，不使用 AttackHitbox3D 节点。
--- 通过参考节点获取物理世界，在指定位置以指定朝向进行圆柱检测。
--- forward 为零向量时报错。
--- callback 返回 false 可提前终止迭代。
---@param ref_node_id integer 参考节点的 ObjectID（用于获取 World3D）
---@param pos_x number 圆柱中心 X
---@param pos_y number 圆柱中心 Y
---@param pos_z number 圆柱中心 Z
---@param forward_x number 朝向向量 X（指向 +Z 方向）
---@param forward_y number 朝向向量 Y
---@param forward_z number 朝向向量 Z
---@param radius number 圆柱半径
---@param height number 圆柱高度
---@param angle number 扇形角度（360=完整圆柱）
---@param collision_mask integer 碰撞层掩码，0 表示检测所有层
---@param callback fun(target_id: integer): boolean 回调函数，返回 false 终止迭代
function M.intersect_cylinder(ref_node_id, pos_x, pos_y, pos_z, forward_x, forward_y, forward_z, radius, height, angle, collision_mask, callback) end

--- native_collision.intersect_box(ref_node_id, pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, size_x, size_y, size_z, collision_mask, callback) -> void
--- 对指定位置执行立方体空间检测，不使用 AttackHitbox3D 节点。
--- 通过参考节点获取物理世界，在指定位置以指定旋转进行立方体检测。
--- callback 返回 false 可提前终止迭代。
---@param ref_node_id integer 参考节点的 ObjectID（用于获取 World3D）
---@param pos_x number 立方体中心 X
---@param pos_y number 立方体中心 Y
---@param pos_z number 立方体中心 Z
---@param rot_x number 绕 X 轴旋转（弧度）
---@param rot_y number 绕 Y 轴旋转（弧度）
---@param rot_z number 绕 Z 轴旋转（弧度）
---@param size_x number 立方体 X 尺寸
---@param size_y number 立方体 Y 尺寸
---@param size_z number 立方体 Z 尺寸
---@param collision_mask integer 碰撞层掩码，0 表示检测所有层
---@param callback fun(target_id: integer): boolean 回调函数，返回 false 终止迭代
function M.intersect_box(ref_node_id, pos_x, pos_y, pos_z, rot_x, rot_y, rot_z, size_x, size_y, size_z, collision_mask, callback) end

--- native_collision.set_hitbox_active(node_id, active) -> void
--- 设置指定节点或其子节点中的 AttackHitbox3D 的 active 状态。
--- 如果 node_id 本身是 AttackHitbox3D，直接设置；
--- 否则遍历其直接子节点（仅一层），找到所有 AttackHitbox3D 进行批量设置。
--- active 状态会影响 hitbox 运行时调试填充的颜色显示。
---@param node_id integer 节点的 ObjectID（可以是 AttackHitbox3D 或包含 AttackHitbox3D 子节点的父节点）
---@param active boolean 是否激活状态
function M.set_hitbox_active(node_id, active) end

return M
