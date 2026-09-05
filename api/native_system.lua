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

--- native_system.get_unique_id() -> string
--- 获取设备唯一标识符。
--- 注意：该字符串在重装系统、升级或修改硬件后可能变化，不可用于持久数据加密；也可能被外部程序伪造，不可用于安全校验。
--- OS 不可用时返回空串。
---@return string unique_id 设备唯一标识符
function M.get_unique_id() end

--- native_system.get_locale() -> string
--- 获取宿主操作系统的区域设置（locale），与 Godot 的 OS.get_locale() 一致。
--- 返回形如 language_Script_COUNTRY_VARIANT@extra 的字符串，language 之后的部分均为可选。
--- 如需仅获取语言代码，可使用 OS.get_locale_language()。
--- OS 不可用时返回空串。
---@return string locale 宿主操作系统的区域设置字符串
function M.get_locale() end

return M
