---@meta

---@class native_network
local M = {}

-- ============================================================================
-- 模块初始化
-- ============================================================================

--- native_network.init(root_name?) -> bool
--- 初始化网络模块，创建容器节点。
--- 必须在场景树就绪后调用（如第一帧 update 回调中）。
---@param root_name? string 容器节点名称，默认 "_NetworkRoot"
---@return boolean success 是否成功
function M.init(root_name) end

--- native_network.is_initialized() -> bool
--- 查询模块是否已初始化。
---@return boolean initialized 是否已初始化
function M.is_initialized() end

-- ============================================================================
-- 请求生命周期
-- ============================================================================

--- native_network.http_post(url, content_type, body) -> id, error
--- 发起 HTTP POST 请求，内部创建 HTTPRequest 节点挂载到容器。
--- 必须先调用 init() 初始化模块。
---@param url string 目标 URL
---@param content_type string Content-Type，如 "application/json"
---@param body string 请求体
---@return integer id 请求 ID，发起失败返回 -1
---@return integer error 错误码，0 表示成功（Error.OK）
function M.http_post(url, content_type, body) end

--- native_network.is_http_post_completed(request_id) -> bool
--- 查询指定请求是否已完成（无论成功或失败）。
---@param request_id integer 请求 ID
---@return boolean completed 是否已完成
function M.is_http_post_completed(request_id) end

--- native_network.destroy_request(request_id) -> void
--- 销毁请求，断开信号并从场景树移除节点。
--- Lua 侧不依赖 GC，请求必须显式销毁。
---@param request_id integer 请求 ID
---@return nil id 无效时通常会被底层忽略
function M.destroy_request(request_id) end

return M