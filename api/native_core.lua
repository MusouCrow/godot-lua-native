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

return M
