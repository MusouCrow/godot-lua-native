---@meta

---@class native_core
local M = {}

--- native_core.bind_update(func) -> void
--- 绑定 update 回调函数。
--- 该函数将在每个物理帧被调用。
---@param func fun(delta: number): void 接收 delta 参数的回调函数
function M.bind_update(func) end

--- native_core.bind_shutdown(func) -> void
--- 绑定 shutdown 回调函数。
--- 该函数将在游戏退出时被调用。
---@param func fun(): void 无参回调函数
function M.bind_shutdown(func) end

return M
