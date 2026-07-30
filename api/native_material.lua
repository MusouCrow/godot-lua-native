---@meta

---@class native_material
local M = {}

--- native_material.set_param_color(id, param_name, r, g, b, a) -> boolean
--- 在节点的直接子节点中设置实例着色器颜色参数。
---@param id integer 节点句柄
---@param param_name string 实例着色器参数名
---@param r number 红色分量
---@param g number 绿色分量
---@param b number 蓝色分量
---@param a number alpha 分量
---@return boolean success 是否至少有一个子节点应用成功
function M.set_param_color(id, param_name, r, g, b, a) end

--- native_material.set_material_override(node_id, material_path) -> integer
--- 设置节点自身及其直接子节点的 material_override 属性。
--- material_path 使用资源路径，如 "res://materials/mat_afterimage.tres"
---@param node_id integer 节点句柄
---@param material_path string 材质资源路径
---@return integer count 设置成功的 GeometryInstance3D 节点数量
function M.set_material_override(node_id, material_path) end

--- native_material.set_transparency(node_id, transparency) -> integer
--- 设置节点自身及其直接子节点的 transparency 属性（0.0=不透明，1.0=完全透明）。
---@param node_id integer 节点句柄
---@param transparency number 透明度值
---@return integer count 设置成功的 GeometryInstance3D 节点数量
function M.set_transparency(node_id, transparency) end

return M
