-- path: tests/test_input.lua
-- desc: native_input 模块测试

local _assert = require('tests.assert')
local _input = require('native_input')

-- Local Cache
local type = type
local print = print

-- 测试 is_pressed 返回值类型
local function _test_is_pressed_type()
	_assert.set_current_test('test_is_pressed_type')

	local result = _input.is_pressed('nonexistent_action')

	_assert.assert_eq(type(result), 'boolean', 'is_pressed should return boolean')

	print('test_is_pressed_type: OK')
end

-- 测试 is_hold 返回值类型
local function _test_is_hold_type()
	_assert.set_current_test('test_is_hold_type')

	local result = _input.is_hold('nonexistent_action')

	_assert.assert_eq(type(result), 'boolean', 'is_hold should return boolean')

	print('test_is_hold_type: OK')
end

-- 测试 is_released 返回值类型
local function _test_is_released_type()
	_assert.set_current_test('test_is_released_type')

	local result = _input.is_released('nonexistent_action')

	_assert.assert_eq(type(result), 'boolean', 'is_released should return boolean')

	print('test_is_released_type: OK')
end

-- 测试 get_strength 返回值类型和范围
local function _test_get_strength_type()
	_assert.set_current_test('test_get_strength_type')

	local result = _input.get_strength('nonexistent_action')

	_assert.assert_eq(type(result), 'number', 'get_strength should return number')
	_assert.assert_true(result >= 0.0, 'get_strength should be >= 0.0')
	_assert.assert_true(result <= 1.0, 'get_strength should be <= 1.0')

	print('test_get_strength_type: OK')
end

-- 测试 get_axis 返回值类型和范围
local function _test_get_axis_type()
	_assert.set_current_test('test_get_axis_type')

	local result = _input.get_axis('nonexistent_neg', 'nonexistent_pos')

	_assert.assert_eq(type(result), 'number', 'get_axis should return number')
	_assert.assert_true(result >= -1.0, 'get_axis should be >= -1.0')
	_assert.assert_true(result <= 1.0, 'get_axis should be <= 1.0')

	print('test_get_axis_type: OK')
end

-- 测试 get_vector 返回值类型
local function _test_get_vector_type()
	_assert.set_current_test('test_get_vector_type')

	local x, y = _input.get_vector('l', 'r', 'u', 'd')

	_assert.assert_eq(type(x), 'number', 'get_vector x should be number')
	_assert.assert_eq(type(y), 'number', 'get_vector y should be number')

	print('test_get_vector_type: OK')
end

-- 测试 bind_input 接受函数参数
local function _test_bind_input_accepts_function()
	_assert.set_current_test('test_bind_input_accepts_function')

	-- 绑定一个空函数应该不报错
	local called = false
	local function dummy_callback(action, pressed, strength, device_type)
		called = true
	end

	-- 这里只测试调用不报错
	_input.bind_input(dummy_callback)

	print('test_bind_input_accepts_function: OK')
end

-- 测试未注册 action 返回 false/0
local function _test_nonexistent_action_defaults()
	_assert.set_current_test('test_nonexistent_action_defaults')

	_assert.assert_false(_input.is_pressed('__nonexistent__'), 'nonexistent action is_pressed should be false')
	_assert.assert_false(_input.is_hold('__nonexistent__'), 'nonexistent action is_hold should be false')
	_assert.assert_false(_input.is_released('__nonexistent__'), 'nonexistent action is_released should be false')
	_assert.assert_eq(0.0, _input.get_strength('__nonexistent__'), 'nonexistent action strength should be 0')

	print('test_nonexistent_action_defaults: OK')
end

-- 执行所有测试
_test_is_pressed_type()
_test_is_hold_type()
_test_is_released_type()
_test_get_strength_type()
_test_get_axis_type()
_test_get_vector_type()
_test_bind_input_accepts_function()
_test_nonexistent_action_defaults()
