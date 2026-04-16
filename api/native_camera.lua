---@meta

---@class native_camera
local M = {}

--- native_camera.set_fov(id, fov) -> void
--- 设置 Camera3D 的视场角。
---@param id integer 节点句柄
---@param fov number 视场角（度）
function M.set_fov(id, fov) end

--- native_camera.get_fov(id) -> number
--- 获取 Camera3D 的视场角。
---@param id integer 节点句柄
---@return number fov
function M.get_fov(id) end

return M
