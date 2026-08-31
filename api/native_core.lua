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

--- native_core.string_hash(str) -> integer
--- 计算字符串的哈希值，与 Godot 的 String.hash() 一致。
---@param str string 待计算哈希的字符串
---@return integer 32 位哈希数值
function M.string_hash(str) end

return M
