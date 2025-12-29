-- path: tests/test_display.lua
-- desc: native_display 模块测试

local _assert = require('tests.assert')
local _display = require('native_display')

-- Local Cache
local type = type
local tostring = tostring
local print = print

-- 测试获取窗口尺寸
local function _test_window_get_size()
	_assert.set_current_test('test_window_get_size')

	local w, h = _display.window_get_size()

	-- 尺寸应为数字类型
	_assert.assert_eq(type(w), 'number', 'width should be a number')
	_assert.assert_eq(type(h), 'number', 'height should be a number')

	-- 尺寸应非负（headless 模式下可能为 0）
	_assert.assert_true(w >= 0, 'width should be >= 0')
	_assert.assert_true(h >= 0, 'height should be >= 0')

	print('test_window_get_size: OK')
end

-- 测试设置有效窗口尺寸
local function _test_window_set_size_valid()
	_assert.set_current_test('test_window_set_size_valid')

	-- 尝试设置尺寸（窗口模式下应成功）
	-- 注意：headless 模式可能因模式限制而失败
	local rc = _display.window_set_size(800, 600)

	-- 返回码应为数字
	_assert.assert_eq(type(rc), 'number', 'return code should be a number')

	-- 不断言 rc == 0，因为 headless 模式可能不支持窗口操作
	print('test_window_set_size_valid: OK (rc=' .. tostring(rc) .. ')')
end

-- 测试设置零尺寸（应失败）
local function _test_window_set_size_invalid_zero()
	_assert.set_current_test('test_window_set_size_invalid_zero')

	-- 零宽度应失败
	local rc = _display.window_set_size(0, 600)
	_assert.assert_ne(rc, 0, 'zero width should return non-zero error code')

	-- 零高度应失败
	rc = _display.window_set_size(800, 0)
	_assert.assert_ne(rc, 0, 'zero height should return non-zero error code')

	-- 双零应失败
	rc = _display.window_set_size(0, 0)
	_assert.assert_ne(rc, 0, 'zero size should return non-zero error code')

	print('test_window_set_size_invalid_zero: OK')
end

-- 测试设置负尺寸（应失败）
local function _test_window_set_size_invalid_negative()
	_assert.set_current_test('test_window_set_size_invalid_negative')

	-- 负宽度应失败
	local rc = _display.window_set_size(-1, 600)
	_assert.assert_ne(rc, 0, 'negative width should return non-zero error code')

	-- 负高度应失败
	rc = _display.window_set_size(800, -100)
	_assert.assert_ne(rc, 0, 'negative height should return non-zero error code')

	print('test_window_set_size_invalid_negative: OK')
end

-- 执行所有测试
_test_window_get_size()
_test_window_set_size_valid()
_test_window_set_size_invalid_zero()
_test_window_set_size_invalid_negative()
