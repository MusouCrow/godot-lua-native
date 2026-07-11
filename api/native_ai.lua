---@meta native_ai

---@brief AI模块：NavigationAgent3D封装
---
---提供AI单位寻路和避障功能。
---创建NavigationAgent3D节点、设置目标位置、获取路径位置、
---设置速度并获取避障计算后的安全速度。
local native_ai = {}

---创建NavigationAgent3D并挂载到父节点。
---
---避免默认开启（avoidance_enabled = true）。
---@param parent_node_id integer 父节点ID（来自`native_node`模块）
---@return integer agent_id，失败返回-1
function native_ai.create(parent_node_id) end

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

---设置速度。
---
---设置期望速度用于避障计算。
---避障系统会尝试满足此速度，但可能因其他代理或障碍物而修改。
---@param agent_id integer AI代理ID
---@param vx number X方向速度分量
---@param vy number Y方向速度分量
---@param vz number Z方向速度分量
function native_ai.set_velocity(agent_id, vx, vy, vz) end

---获取安全速度。
---
---读取避障计算后的安全速度（由`velocity_computed`信号更新）。
---当avoidance_enabled为true时有实际意义。
---@param agent_id integer AI代理ID
---@return number vx 安全速度X分量
---@return number vy 安全速度Y分量
---@return number vz 安全速度Z分量
function native_ai.get_safe_velocity(agent_id) end

return native_ai
