---@meta native_ai

---@brief AI模块：NavigationAgent3D封装
---
---提供AI单位寻路功能。
---创建NavigationAgent3D节点、设置目标位置、获取路径位置。
local native_ai = {}

---创建NavigationAgent3D并挂载到父节点。
---
---@param parent_node_id integer 父节点ID（来自`native_node`模块）
---@param path_desired_distance number 路径期望距离（单位：米）
---@param target_desired_distance number 目标期望距离（单位：米）
---@return integer agent_id，失败返回-1
function native_ai.create(parent_node_id, path_desired_distance, target_desired_distance) end

---销毁NavigationAgent3D。
---@param agent_id integer AI代理ID
function native_ai.destroy(agent_id) end

---设置目标位置。
---如果设置，导航代理将从当前位置向目标位置请求新路径。
---@param agent_id integer AI代理ID
---@param x number 目标X坐标
---@param y number 目标Y坐标
---@param z number 目标Z坐标
function native_ai.set_target_position(agent_id, x, y, z) end

---获取下一个路径位置。
---
---**必须每物理帧调用一次**以更新内部路径逻辑。
---返回值表示下一个可移动到的全局坐标位置。
---@param agent_id integer AI代理ID
---@return number x 下一个位置X坐标
---@return number y 下一个位置Y坐标
---@return number z 下一个位置Z坐标
function native_ai.get_next_path_position(agent_id) end

---判断导航是否完成。
---
---返回true表示导航已完成（到达目标或最后路径点）。
---注意：返回true时应停止调用`get_next_path_position`，避免站立抖动。
---@param agent_id integer AI代理ID
---@return boolean completed 是否完成
function native_ai.is_navigation_finished(agent_id) end

---查询导航网格上离给定坐标最近的点。
---
---将任意空间坐标投影到可导航的表面上。
---@param agent_id integer AI代理ID（用于获取navigation map）
---@param x number 查询点X坐标
---@param y number 查询点Y坐标
---@param z number 查询点Z坐标
---@return number closest_x 导航网格上最近点X坐标
---@return number closest_y 导航网格上最近点Y坐标
---@return number closest_z 导航网格上最近点Z坐标
function native_ai.map_get_closest_point(agent_id, x, y, z) end

return native_ai
