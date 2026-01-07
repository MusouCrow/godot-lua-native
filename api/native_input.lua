---@meta

---@alias input_callback fun(action: string, pressed: boolean, strength: number, device_type: string)

---@class native_input
local M = {}

--- native_input.bind_input(func) -> void
--- 绑定输入回调函数。
--- func: 接收 (action, pressed, strength, device_type) 参数的函数。
---@param func input_callback
function M.bind_input(func) end

--- native_input.is_pressed(action) -> bool
--- 返回 action 是否刚刚按下（按下瞬间）。
---@param action string
---@return boolean
function M.is_pressed(action) end

--- native_input.is_hold(action) -> bool
--- 返回 action 是否持续按住。
---@param action string
---@return boolean
function M.is_hold(action) end

--- native_input.is_released(action) -> bool
--- 返回 action 是否刚刚弹起（弹起瞬间）。
---@param action string
---@return boolean
function M.is_released(action) end

--- native_input.get_strength(action) -> float
--- 返回 action 的力度 (0.0~1.0)。
---@param action string
---@return number
function M.get_strength(action) end

--- native_input.get_axis(neg_action, pos_action) -> float
--- 返回轴值 (-1.0~1.0)。
---@param neg_action string
---@param pos_action string
---@return number
function M.get_axis(neg_action, pos_action) end

--- native_input.get_vector(left, right, up, down) -> x, y
--- 返回归一化 2D 向量。
---@param left string
---@param right string
---@param up string
---@param down string
---@return number x
---@return number y
function M.get_vector(left, right, up, down) end

--- native_input.vibrate(weak, strong, duration) -> void
--- 触发手柄震动。
---@param weak number 弱马达强度 (0.0~1.0)
---@param strong number 强马达强度 (0.0~1.0)
---@param duration number 持续秒数
function M.vibrate(weak, strong, duration) end

return M
