---@meta

---@class native_node
local M = {}

-- ============================================================================
-- 节点引用管理
-- ============================================================================

--- native_node.set_root(path) -> boolean
--- 设置根节点路径，后续创建的节点将挂载到此节点下。
---@param path string 根节点路径（如 "/root/pre_entry/pre_scene/root"）
---@return boolean success 是否设置成功
function M.set_root(path) end

--- native_node.instantiate(scene_path) -> int
--- 从场景资源路径加载并实例化节点，挂载到根节点下。
--- 注意：必须先调用 set_root 设置根节点。
---@param scene_path string 场景资源路径（如 "res://assets/char/sm_char_proto.glb"）
---@return integer id 节点句柄，失败返回 -1
function M.instantiate(scene_path) end

--- native_node.destroy(id) -> void
--- 销毁节点并释放引用。
--- 对于通过 instantiate 创建的节点，调用 queue_free 销毁节点。
--- 对于通过 get_child_by_path 获取的节点，仅释放引用。
---@param id integer 节点句柄
---@return nil id 无效时通常会被底层忽略
function M.destroy(id) end

--- native_node.get_child_by_path(id, path) -> int
--- 基于指定节点查找子节点并返回句柄。
---@param id integer 父节点句柄
---@param path string 子节点路径（如 "target" 或 "bones/spine"）
---@return integer child_id 子节点句柄，失败返回 -1
function M.get_child_by_path(id, path) end

--- native_node.is_valid(id) -> boolean
--- 检查节点引用是否有效。
---@param id integer 节点句柄
---@return boolean valid 是否有效
function M.is_valid(id) end

-- ============================================================================
-- 信息
-- ============================================================================

--- native_node.get_name(id) -> string
--- 获取节点名称。
---@param id integer 节点句柄
---@return string name 节点名称
function M.get_name(id) end

--- native_node.get_type(id) -> string
--- 获取节点当前的 Godot 运行时类型。
---@param id integer 节点句柄
---@return string type 节点类型字符串（如 "Node3D"、"CharacterBody3D"、"Camera3D"）
function M.get_type(id) end

return M
