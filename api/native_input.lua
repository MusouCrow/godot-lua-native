---@meta

---@alias input_callback fun(action_name: string, pressed: boolean, strength: number, device_type: string)

---@class native_input
local M = {}

--- native_input.bind_input(func) -> void
--- 绑定输入回调函数。
--- func: 接收 (action_name, pressed, strength, device_type) 参数的函数。
---@param func input_callback
---@return nil 绑定失败时由底层忽略或报错
function M.bind_input(func) end

--- native_input.is_pressed(action_name) -> bool
--- 返回 action_name 是否刚刚按下（按下瞬间）。
---@param action_name string
---@return boolean is_pressed action_name 未绑定时通常返回 false
function M.is_pressed(action_name) end

--- native_input.is_hold(action_name) -> bool
--- 返回 action_name 是否持续按住。
---@param action_name string
---@return boolean is_hold action_name 未绑定时通常返回 false
function M.is_hold(action_name) end

--- native_input.is_released(action_name) -> bool
--- 返回 action_name 是否刚刚弹起（弹起瞬间）。
---@param action_name string
---@return boolean is_released action_name 未绑定时通常返回 false
function M.is_released(action_name) end

--- native_input.get_strength(action_name) -> float
--- 返回 action_name 的力度 (0.0~1.0)。
---@param action_name string
---@return number strength action_name 未绑定时通常返回 0.0
function M.get_strength(action_name) end

--- native_input.get_axis(neg_action_name, pos_action_name) -> float
--- 返回轴值 (-1.0~1.0)。
---@param neg_action_name string
---@param pos_action_name string
---@return number axis 任一 action_name 未绑定时通常按 0.0 处理
function M.get_axis(neg_action_name, pos_action_name) end

--- native_input.get_vector(left_action_name, right_action_name, up_action_name, down_action_name) -> x, y
--- 返回归一化 2D 向量。
---@param left_action_name string
---@param right_action_name string
---@param up_action_name string
---@param down_action_name string
---@return number x action_name 未绑定时通常按 0.0 处理
---@return number y action_name 未绑定时通常按 0.0 处理
function M.get_vector(left_action_name, right_action_name, up_action_name, down_action_name) end

--- native_input.vibrate(weak, strong, duration) -> void
--- 触发手柄震动。
---@param weak number 弱马达强度 (0.0~1.0)
---@param strong number 强马达强度 (0.0~1.0)
---@param duration number 持续秒数
---@return nil 当前设备不支持震动时通常会被底层忽略
function M.vibrate(weak, strong, duration) end

return M
