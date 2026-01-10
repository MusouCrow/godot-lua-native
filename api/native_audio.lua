---@meta

---@class native_audio
local M = {}

-- ============================================================================
-- 模块初始化
-- ============================================================================

--- native_audio.init(root_name?) -> bool
--- 初始化音频模块，创建容器节点。
--- 必须在场景树就绪后调用（如第一帧 update 回调中）。
---@param root_name? string 容器节点名称，默认 "_AudioRoot"
---@return boolean success 是否成功
function M.init(root_name) end

--- native_audio.is_initialized() -> bool
--- 查询模块是否已初始化。
---@return boolean initialized 是否已初始化
function M.is_initialized() end

-- ============================================================================
-- 播放器生命周期
-- ============================================================================

--- native_audio.create_player(is_spatial) -> int
--- 创建音频播放器，挂载到场景树。
--- 必须先调用 init() 初始化模块。
---@param is_spatial boolean false=AudioStreamPlayer（非空间）, true=AudioStreamPlayer3D（空间）
---@return integer id 播放器 ID，失败返回 -1
function M.create_player(is_spatial) end

--- native_audio.destroy_player(id) -> void
--- 销毁播放器，从场景树移除。
---@param id integer 播放器 ID
function M.destroy_player(id) end

-- ============================================================================
-- 播放器操作
-- ============================================================================

--- native_audio.set_stream(id, path) -> bool
--- 加载音频资源到播放器。
---@param id integer 播放器 ID
---@param path string 资源路径（如 "res://audio/music.ogg"）
---@return boolean success 加载成功返回 true
function M.set_stream(id, path) end

--- native_audio.play(id) -> void
--- 播放音频。
---@param id integer 播放器 ID
function M.play(id) end

--- native_audio.stop(id) -> void
--- 停止播放。
---@param id integer 播放器 ID
function M.stop(id) end

--- native_audio.is_playing(id) -> bool
--- 查询播放器是否正在播放。
---@param id integer 播放器 ID
---@return boolean playing 是否正在播放
function M.is_playing(id) end

--- native_audio.set_paused(id, paused) -> void
--- 设置播放器暂停状态。
---@param id integer 播放器 ID
---@param paused boolean 是否暂停
function M.set_paused(id, paused) end

--- native_audio.is_paused(id) -> bool
--- 查询播放器是否暂停。
---@param id integer 播放器 ID
---@return boolean paused 是否暂停
function M.is_paused(id) end

--- native_audio.set_volume(id, volume) -> void
--- 设置播放器音量（线性 0-1）。
---@param id integer 播放器 ID
---@param volume number 音量（线性 0-1），1.0 为原始音量，0 为静音
function M.set_volume(id, volume) end

--- native_audio.set_pitch(id, pitch) -> void
--- 设置播放器音调倍率。
---@param id integer 播放器 ID
---@param pitch number 音调倍率，1.0 为原始音调
function M.set_pitch(id, pitch) end

--- native_audio.set_bus(id, bus_name) -> void
--- 设置播放器的 AudioBus。
---@param id integer 播放器 ID
---@param bus_name string AudioBus 名称
function M.set_bus(id, bus_name) end

--- native_audio.set_position(id, x, y, z) -> void
--- 设置空间音频播放器的 3D 位置（仅对空间播放器有效）。
---@param id integer 播放器 ID
---@param x number X 坐标
---@param y number Y 坐标
---@param z number Z 坐标
function M.set_position(id, x, y, z) end

--- native_audio.set_loop(id, enabled) -> void
--- 设置音频循环播放（仅支持 MP3 和 OGG 格式）。
---@param id integer 播放器 ID
---@param enabled boolean 是否循环
function M.set_loop(id, enabled) end

-- ============================================================================
-- AudioBus 操作
-- ============================================================================

--- native_audio.add_bus(name) -> int
--- 添加新的 AudioBus，发送到 Master。
--- 如果同名 bus 已存在，返回已有 bus 的 index。
---@param name string AudioBus 名称
---@return integer index Bus 索引，失败返回 -1
function M.add_bus(name) end

--- native_audio.set_bus_volume(name, volume) -> void
--- 设置 AudioBus 音量（线性 0-1）。
---@param name string AudioBus 名称
---@param volume number 音量（线性 0-1）
function M.set_bus_volume(name, volume) end

--- native_audio.get_bus_volume(name) -> number
--- 获取 AudioBus 音量（线性 0-1）。
---@param name string AudioBus 名称
---@return number volume 音量（线性 0-1）
function M.get_bus_volume(name) end

--- native_audio.set_master_volume(volume) -> void
--- 设置主音量（线性 0-1）。
---@param volume number 音量（线性 0-1）
function M.set_master_volume(volume) end

--- native_audio.get_master_volume() -> number
--- 获取主音量（线性 0-1）。
---@return number volume 音量（线性 0-1）
function M.get_master_volume() end

return M
