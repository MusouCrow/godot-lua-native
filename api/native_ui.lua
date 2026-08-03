---@meta

---@class native_ui
local M = {}

-- ============================================================================
-- CanvasItem
-- ============================================================================

--- native_ui.get_visible(handle) -> boolean
--- 获取 CanvasItem 的可见性。
---@param handle integer native_node 节点句柄
---@return boolean visible 是否可见；节点无效时返回 false
function M.get_visible(handle) end

--- native_ui.set_visible(handle, visible) -> void
--- 设置 CanvasItem 的可见性。
---@param handle integer native_node 节点句柄
---@param visible boolean 是否可见
function M.set_visible(handle, visible) end

--- native_ui.set_modulate(handle, r, g, b, a) -> void
--- 设置 CanvasItem 的调制颜色，会传递给子 CanvasItem。
---@param handle integer native_node 节点句柄
---@param r number 红色分量 [0.0, 1.0]
---@param g number 绿色分量 [0.0, 1.0]
---@param b number 蓝色分量 [0.0, 1.0]
---@param a number 透明度分量 [0.0, 1.0]
function M.set_modulate(handle, r, g, b, a) end

-- ============================================================================
-- Control
-- ============================================================================

--- native_ui.get_position(handle) -> (x, y)
--- 获取 Control 的位置。
---@param handle integer native_node 节点句柄
---@return number x X 坐标
---@return number y Y 坐标；节点无效时返回 (0, 0)
function M.get_position(handle) end

--- native_ui.set_position(handle, x, y) -> void
--- 设置 Control 的位置。
---@param handle integer native_node 节点句柄
---@param x number X 坐标
---@param y number Y 坐标
function M.set_position(handle, x, y) end

--- native_ui.get_scale(handle) -> (x, y)
--- 获取 Control 的缩放。
---@param handle integer native_node 节点句柄
---@return number x X 轴缩放
---@return number y Y 轴缩放；节点无效时返回 (0, 0)
function M.get_scale(handle) end

--- native_ui.set_scale(handle, x, y) -> void
--- 设置 Control 的缩放。
---@param handle integer native_node 节点句柄
---@param x number X 轴缩放
---@param y number Y 轴缩放
function M.set_scale(handle, x, y) end

-- ============================================================================
-- Range
-- ============================================================================

--- native_ui.set_value(handle, value) -> void
--- 设置 Range 的值，会触发 value_changed 信号。
---@param handle integer native_node 节点句柄
---@param value number 数值
function M.set_value(handle, value) end

return M
