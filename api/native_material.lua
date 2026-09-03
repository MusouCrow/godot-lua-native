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

--- native_material.set_param_vec3(id, param_name, x, y, z) -> boolean
--- 在节点的直接子节点中设置实例着色器Vector3参数。
---@param id integer 节点句柄
---@param param_name string 实例着色器参数名
---@param x number X分量
---@param y number Y分量
---@param z number Z分量
---@return boolean success 是否至少有一个子节点应用成功
function M.set_param_vec3(id, param_name, x, y, z) end

--- native_material.set_param_float(id, param_name, value) -> boolean
--- 在节点的直接子节点中设置实例着色器float参数。
---@param id integer 节点句柄
---@param param_name string 实例着色器参数名
---@param value number 参数值
---@return boolean success 是否至少有一个子节点应用成功
function M.set_param_float(id, param_name, value) end

--- native_material.set_global_param_float(param_name, value) -> nil
--- 设置全局着色器float参数（基于RenderingServer.global_shader_parameter_set）。
---@param param_name string 全局着色器参数名
---@param value number 参数值
function M.set_global_param_float(param_name, value) end

--- native_material.set_global_param_vec3(param_name, x, y, z) -> nil
--- 设置全局着色器Vector3参数（基于RenderingServer.global_shader_parameter_set）。
---@param param_name string 全局着色器参数名
---@param x number X分量
---@param y number Y分量
---@param z number Z分量
function M.set_global_param_vec3(param_name, x, y, z) end

--- native_material.set_global_param_color(param_name, r, g, b, a) -> nil
--- 设置全局着色器颜色参数（基于RenderingServer.global_shader_parameter_set）。
---@param param_name string 全局着色器参数名
---@param r number 红色分量
---@param g number 绿色分量
---@param b number 蓝色分量
---@param a number alpha 分量
function M.set_global_param_color(param_name, r, g, b, a) end

--- native_material.set_material_override(node_id, material_path) -> integer
--- 设置节点自身及其直接子节点的 material_override 属性。
--- material_path 使用资源路径，如 "res://materials/mat_afterimage.tres"
---@param node_id integer 节点句柄
---@param material_path string 材质资源路径
---@return integer count 设置成功的 GeometryInstance3D 节点数量
function M.set_material_override(node_id, material_path) end

--- native_material.set_material_overlay(node_id, material_path) -> integer
--- 设置节点自身及其直接子节点的 material_overlay 属性。
--- material_path 为 nil 时清空 overlay。
---@param node_id integer 节点句柄
---@param material_path string|nil 材质资源路径，nil表示清空overlay
---@return integer count 设置成功的 GeometryInstance3D 节点数量
function M.set_material_overlay(node_id, material_path) end

--- native_material.set_transparency(node_id, transparency) -> integer
--- 设置节点自身及其直接子节点的透明度（0.0=不透明，1.0=完全透明）。
--- Decal 节点没有 transparency 属性，改为设置 albedo_mix（albedo_mix = 1 - transparency）。
---@param node_id integer 节点句柄
---@param transparency number 透明度值
---@return integer count 设置成功的节点数量
function M.set_transparency(node_id, transparency) end

--- native_material.enable_cast_shadow(node_id, enabled) -> integer
--- 设置节点自身及其直接子节点的阴影投射开关。
---@param node_id integer 节点句柄
---@param enabled boolean true=投射阴影，false=关闭阴影
---@return integer count 设置成功的 GeometryInstance3D 节点数量
function M.enable_cast_shadow(node_id, enabled) end

--- native_material.duplicate_materials(node_id) -> integer
--- 复制节点自身及其直接子节点（MeshInstance3D）的 material_override 与所有 surface_override_material。
--- 浅复制材质，避免材质复用导致动画驱动产生问题。
---@param node_id integer 节点句柄
---@return integer count 处理的 MeshInstance3D 节点数量
function M.duplicate_materials(node_id) end

return M
