---@meta

---@class native_core
local M = {}

--- native_core.bind_update(func) -> void
--- 绑定 update 回调函数。
--- 该函数将在每个物理帧被调用。
---@param func fun(delta: number): void 接收 delta 参数的回调函数
---@return nil 绑定失败时由底层忽略或报错
function M.bind_update(func) end

--- native_core.bind_shutdown(func) -> void
--- 绑定 shutdown 回调函数。
--- 该函数将在游戏退出时被调用。
---@param func fun(): void 无参回调函数
---@return nil 绑定失败时由底层忽略或报错
function M.bind_shutdown(func) end

--- native_core.bind_fatal(func) -> void
--- 绑定致命错误善后回调函数。
--- 该函数将在 Lua 运行时销毁前被调用，用于同步上报报错文本与录像队列。
---@param func fun(message: string): void 接收报错文本的回调函数
---@return nil 绑定失败时由底层忽略或报错
function M.bind_fatal(func) end

--- native_core.quit(exit_code) -> void
--- 请求优雅退出。
---@param exit_code? integer 退出码，默认 0
---@return nil 退出请求可能被宿主延后处理
function M.quit(exit_code) end

--- native_core.set_time_scale(scale) -> void
--- 设置游戏时间缩放比例。
--- 影响所有使用 delta 时间的模拟（如 Timer、动画、物理等）。
---@param scale number 时间缩放倍率，1.0 为正常速度，2.0 为两倍速，0.5 为半速
---@return nil
function M.set_time_scale(scale) end

--- native_core.get_time_scale() -> number
--- 获取当前游戏时间缩放比例。
---@return number 当前时间缩放倍率
function M.get_time_scale() end

--- native_core.get_root_path() -> string
--- 获取项目根目录的绝对路径。
---@return string 项目根目录的绝对路径
function M.get_root_path() end

--- native_core.get_unique_id() -> string
--- 获取设备唯一标识符。
--- 注意：该字符串在重装系统、升级或修改硬件后可能变化，不可用于持久数据加密；也可能被外部程序伪造，不可用于安全校验。
---@return string 设备唯一标识符
function M.get_unique_id() end

--- native_core.get_locale() -> string
--- 获取宿主操作系统的区域设置（locale），与 Godot 的 OS.get_locale() 一致。
--- 返回形如 language_Script_COUNTRY_VARIANT@extra 的字符串，language 之后的部分均为可选。
--- 如需仅获取语言代码，可使用 OS.get_locale_language()。
---@return string 宿主操作系统的区域设置字符串
function M.get_locale() end

--- native_core.string_hash(str) -> integer
--- 计算字符串的哈希值，与 Godot 的 String.hash() 一致。
---@param str string 待计算哈希的字符串
---@return integer 32 位哈希数值
function M.string_hash(str) end

--- native_core.load_packed_lua(dat_path, module_name) -> loader, loader_data
--- 从 lua.dat 中加载一个 Lua 字节码模块。
--- 成功时返回可执行的 loader 和 loader data。
--- 失败时返回 nil 和错误文本。
---@param dat_path string lua.dat 的 Godot 路径，例如 res://lua.dat
---@param module_name string Lua 模块名，例如 svc.level_svc
---@return function|nil loader
---@return string loader_data_or_error
function M.load_packed_lua(dat_path, module_name) end

return M
