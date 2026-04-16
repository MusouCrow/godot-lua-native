---@meta

---@class native_material
local M = {}

--- native_material.set_param_color_in_group(id, group_name, param_name, r, g, b, a) -> boolean
--- 在节点子树内按组写入实例着色器颜色参数。
---@param id integer 根节点句柄
---@param group_name string 节点组名称
---@param param_name string 实例着色器参数名
---@param r number 红色分量
---@param g number 绿色分量
---@param b number 蓝色分量
---@param a number alpha 分量
---@return boolean success
function M.set_param_color_in_group(id, group_name, param_name, r, g, b, a) end

return M
