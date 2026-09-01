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

--- native_network.http_post(url, content_type, authorization, body, timeout?) -> id, error
--- 发起 HTTP POST 请求，内部创建 HTTPRequest 节点挂载到容器。
--- 必须先调用 init() 初始化模块。
---@param url string 目标 URL
---@param content_type string Content-Type，如 "application/json"
---@param authorization string Authorization 头内容（如 "Bearer xxx"），空字符串时不携带该头
---@param body string 请求体
---@param timeout? number 超时秒数，0 表示不超时，默认 10.0
---@return integer id 请求 ID，发起失败返回 -1
---@return integer error 错误码，0 表示成功（Error.OK）
function M.http_post(url, content_type, authorization, body, timeout) end

--- native_network.get_http_post_status(request_id) -> completed, result, response_code, body
--- 查询指定请求的完成状态、传输结果、HTTP 状态码与响应体。
--- result 为 Godot HTTPRequest.Result，0 表示传输成功。
--- response_code 为 HTTP 状态码，传输层失败时通常为 0。
--- body 为响应体字节串；未完成或无效 id 时返回空串。
---@param request_id integer 请求 ID
---@return boolean completed 是否已完成
---@return integer result 传输结果（HTTPRequest.Result）
---@return integer response_code HTTP 状态码
---@return string body 响应体
function M.get_http_post_status(request_id) end

--- native_network.destroy_request(request_id) -> void
--- 销毁请求，断开信号并从场景树移除节点。
--- Lua 侧不依赖 GC，请求必须显式销毁。
---@param request_id integer 请求 ID
---@return nil id 无效时通常会被底层忽略
function M.destroy_request(request_id) end

--- native_network.http_post_sync(url, content_type, authorization, body, timeout?) -> bool
--- 阻塞式 POST，仅确保请求字节发出，不读响应体。仅供关闭时同步 flush 使用。
--- 仅支持 http://，遇 https:// 直接失败。
---@param url string 目标 URL（仅 http://）
---@param content_type string Content-Type，如 "application/octet-stream"
---@param authorization string Authorization 头内容（如 "Bearer xxx"），空字符串时不携带该头
---@param body string 请求体
---@param timeout? number 超时秒数，默认 1.0
---@return boolean success 请求字节是否成功发出
function M.http_post_sync(url, content_type, authorization, body, timeout) end

return M