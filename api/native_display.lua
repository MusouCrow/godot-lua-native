---@meta

---@class native_display
local M = {}

---@type integer
M.WINDOW_MODE_WINDOWED = 0
---@type integer
M.WINDOW_MODE_MINIMIZED = 1
---@type integer
M.WINDOW_MODE_MAXIMIZED = 2
---@type integer
M.WINDOW_MODE_FULLSCREEN = 3
---@type integer
M.WINDOW_MODE_EXCLUSIVE_FULLSCREEN = 4

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

--- native.display.window_get_mode() -> mode:int
--- 返回：当前窗口模式（见 WINDOW_MODE_*）；DisplayServer 不可用时返回 -1。
---@return integer mode
function M.window_get_mode() end

--- native.display.window_set_mode(mode:int) -> rc:int
--- 返回：成功返回 0，失败返回 -1。
--- 约束：mode 必须为合法窗口模式（见 WINDOW_MODE_*）。
--- 对应工程设置 display/window/size/mode。
---@param mode integer
---@return integer rc
function M.window_set_mode(mode) end

--- native.display.window_set_center_position() -> rc:int
--- 返回：成功返回 0，失败返回 -1。
--- 约束：仅窗口模式有效；DisplayServer 不可用时返回 -1。
--- 将主窗口居中到当前屏幕中心，用于从全屏切换到窗口后恢复坐标。
---@return integer rc
function M.window_set_center_position() end

--- native.display.content_scale_set_factor(scale:number) -> rc:int
--- 返回：成功返回 0，失败返回 -1。
--- 约束：scale 必须为正数；根窗口不可用时返回 -1。
--- 设置内容缩放因子，对应项目设置 display/window/stretch/scale。
---@param scale number 内容缩放因子，1.0 为原始大小
---@return integer rc
function M.content_scale_set_factor(scale) end

return M