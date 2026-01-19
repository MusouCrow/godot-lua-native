---@meta

---@class native_res
local M = {}

-- ============================================================================
-- 资源预加载
-- ============================================================================

--- native_res.load(path) -> boolean
--- 预加载资源。
--- 资源加载后会被持有，确保在内存中保持活跃。
---@param path string 资源路径（如 "res://assets/char/sm_char_proto.glb"）
---@return boolean success 是否加载成功
function M.load(path) end

--- native_res.unload(path) -> void
--- 卸载资源，释放引用。
---@param path string 资源路径
function M.unload(path) end

--- native_res.is_loaded(path) -> boolean
--- 检查资源是否已加载。
---@param path string 资源路径
---@return boolean loaded 是否已加载
function M.is_loaded(path) end

return M
