---@meta

---@class native_debug_draw
local M = {}

--- 设置调试绘制根节点。
---@param path string 根节点绝对路径，且必须指向 Node3D
---@return boolean success 是否绑定成功
function M.set_root(path) end

--- 开关调试绘制功能。
---@param enabled boolean 是否启用
---@return boolean success 是否处理成功
function M.set_enabled(enabled) end

--- 查询调试绘制是否启用。
---@return boolean enabled 是否启用
function M.is_enabled() end

--- 设置调试绘制节点可见性。
---@param visible boolean 是否可见
---@return boolean success 是否处理成功
function M.set_visible(visible) end

--- 清空命令缓存和已显示网格。
function M.clear() end

--- 提交当前帧缓存的调试绘制命令。
---@return boolean success 是否提交成功
function M.submit() end

--- 添加一个调试点。
---@param x number
---@param y number
---@param z number
---@param size number 世界单位尺寸
---@param r number
---@param g number
---@param b number
---@param a number
---@param is_xray? boolean 是否穿墙显示
---@return boolean success 是否加入缓存成功
function M.add_point(x, y, z, size, r, g, b, a, is_xray) end

--- 添加一条调试线。
---@param from_x number
---@param from_y number
---@param from_z number
---@param to_x number
---@param to_y number
---@param to_z number
---@param width number 世界单位宽度
---@param r number
---@param g number
---@param b number
---@param a number
---@param is_xray? boolean 是否穿墙显示
---@return boolean success 是否加入缓存成功
function M.add_line(from_x, from_y, from_z, to_x, to_y, to_z, width, r, g, b, a, is_xray) end

--- 添加一个调试圆。
---@param center_x number
---@param center_y number
---@param center_z number
---@param normal_x number
---@param normal_y number
---@param normal_z number
---@param radius number 半径
---@param segments integer 采样段数，最少 3
---@param line_width number 轮廓模式下的世界单位宽度
---@param r number
---@param g number
---@param b number
---@param a number
---@param is_xray? boolean 是否穿墙显示
---@param is_fill? boolean true 表示实心圆面
---@return boolean success 是否加入缓存成功
function M.add_circle(center_x, center_y, center_z, normal_x, normal_y, normal_z, radius, segments, line_width, r, g, b, a, is_xray, is_fill) end

--- 添加一个调试扇形。
---@param center_x number
---@param center_y number
---@param center_z number
---@param normal_x number
---@param normal_y number
---@param normal_z number
---@param dir_x number 扇形中心方向
---@param dir_y number
---@param dir_z number
---@param radius number 半径
---@param angle_degrees number 张角（度）
---@param segments integer 采样段数，最少 1
---@param line_width number 轮廓模式下的世界单位宽度
---@param r number
---@param g number
---@param b number
---@param a number
---@param is_xray? boolean 是否穿墙显示
---@param is_fill? boolean true 表示实心扇面
---@return boolean success 是否加入缓存成功
function M.add_sector(center_x, center_y, center_z, normal_x, normal_y, normal_z, dir_x, dir_y, dir_z, radius, angle_degrees, segments, line_width, r, g, b, a, is_xray, is_fill) end

--- 获取最近一次提交统计。
---@return integer point_commands
---@return integer line_commands
---@return integer circle_commands
---@return integer sector_commands
---@return integer points_vertex_count
---@return integer lines_vertex_count
---@return integer faces_vertex_count
---@return integer submit_count
---@return boolean had_camera
function M.get_stats() end

return M
