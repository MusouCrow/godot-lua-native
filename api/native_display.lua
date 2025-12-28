---@meta

---@class native_display
local M = {}

--- native.display.window_get_size() -> (w:int, h:int)
--- 返回：宽度和高度两个整数；DisplayServer 不可用时返回 (0, 0)。
---@return integer w
---@return integer h
function M.window_get_size() end

--- native.display.window_set_size(w:int, h:int) -> rc:int
--- 返回：成功返回 0，失败返回 -1。
--- 约束：w 和 h 必须为正整数；全屏/最大化模式下无法设置尺寸。
---@param w integer
---@param h integer
---@return integer rc
function M.window_set_size(w, h) end

return M
