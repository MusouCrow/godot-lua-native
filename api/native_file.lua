---@meta

---@class native_file
local M = {}

---@param path string
---@return boolean success
function M.make_dir_recursive(path) end

---@param path string
---@param data string
---@return boolean success
function M.write_file(path, data) end

---@param path string
---@return string|nil data
function M.read_file(path) end

---@param path string
---@param data string
---@return boolean success
function M.append_file(path, data) end

---@param path string
---@return integer|nil handle
function M.open_read(path) end

---@param handle integer
---@param length integer
---@return string|nil data
function M.read(handle, length) end

---@param handle integer
---@return boolean success
function M.close(handle) end

return M