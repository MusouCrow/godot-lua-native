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

return M
