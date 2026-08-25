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

--- native_camera.unproject_position(id, x, y, z) -> screen_x, screen_y
--- 将世界坐标投影为视口内 2D 屏幕坐标。
---@param id integer 节点句柄
---@param x number 世界坐标 X
---@param y number 世界坐标 Y
---@param z number 世界坐标 Z
---@return number screen_x 屏幕坐标 X
---@return number screen_y 屏幕坐标 Y
function M.unproject_position(id, x, y, z) end

--- native_camera.get_ground_footprint_aabb(id, ground_y) -> min_x, max_x, min_z, max_z
--- 将当前 viewport 四角射线投射到 y = ground_y 平面，返回 X/Z 轴对齐包围范围。
--- 覆盖当前 FOV、相机位置、俯角、滚转、宽高比与缩放距离。
--- 任一角无有效交点时返回 4 个 0。
---@param id integer 节点句柄
---@param ground_y number 地面平面的 Y 值
---@return number min_x
---@return number max_x
---@return number min_z
---@return number max_z
function M.get_ground_footprint_aabb(id, ground_y) end

return M
