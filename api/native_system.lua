---@meta

---@class native_system
local M = {}

--- native_system.get_name() -> string
--- 返回操作系统名称。
--- 可能的返回值：
--- - "Windows" (Windows 系统)
--- - "macOS" (macOS 系统)
--- - "Linux" (Linux 系统，包括 Steam Deck)
--- - "Android" (Android 系统)
--- - "iOS" (iOS 系统)
--- - "Web" (Web 平台)
--- - "Unknown" (OS 不可用时)
---@return string name 操作系统名称
function M.get_name() end

--- native_system.get_rendering_method() -> string
--- 返回当前渲染方法。
--- 常见值包括：
--- - "forward_plus"
--- - "mobile"
--- - "gl_compatibility"
--- - "dummy"
--- - "Unknown" (OS 不可用时)
---@return string rendering_method 当前渲染方法
function M.get_rendering_method() end

return M
