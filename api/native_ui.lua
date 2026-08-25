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

-- Control 的 position 和 scale 操作已迁移到 native_transform 模块。

--- native_ui.get_size(handle) -> number, number
--- 获取 Control 节点的尺寸（宽度与高度）。
---@param handle integer native_node 节点句柄
---@return number width 宽度；节点无效时返回 0.0
---@return number height 高度；节点无效时返回 0.0
function M.get_size(handle) end

-- ============================================================================
-- Range
-- ============================================================================

--- native_ui.get_bar_value(handle) -> number
--- 获取 Range 的值（进度条）。
---@param handle integer native_node 节点句柄
---@return number value 当前值；节点无效时返回 0.0
function M.get_bar_value(handle) end

--- native_ui.set_bar_value(handle, value) -> void
--- 设置 Range 的值（进度条），会触发 value_changed 信号。
---@param handle integer native_node 节点句柄
---@param value number 目标值
function M.set_bar_value(handle, value) end

-- ============================================================================
-- RichTextLabel
-- ============================================================================

--- native_ui.get_text(handle) -> string
--- 获取 RichTextLabel 的文本内容。
---@param handle integer native_node 节点句柄
---@return string text 文本内容；节点无效时返回空字符串
function M.get_text(handle) end

--- native_ui.set_text(handle, text) -> void
--- 设置 RichTextLabel 的文本内容。
---@param handle integer native_node 节点句柄
---@param text string 文本内容（支持 BBCode）
function M.set_text(handle, text) end

-- ============================================================================
-- TextureRect
-- ============================================================================

--- native_ui.set_texture(handle, texture_path) -> boolean
--- 设置 TextureRect 的纹理。
---@param handle integer native_node 节点句柄
---@param texture_path string 纹理资源路径
---@return boolean ok 是否设置成功；节点无效或纹理加载失败时返回 false
function M.set_texture(handle, texture_path) end

return M
