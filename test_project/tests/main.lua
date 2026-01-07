-- path: tests/main.lua
-- desc: 测试入口，执行所有测试文件并返回退出码（0 = 成功，非零 = 失败）

local _assert = require('tests.assert')

print('=== Lua Native Test Suite ===')
print('')

-- 基础 Lua 功能测试
local function _test_lua_basic()
	_assert.set_current_test('test_lua_basic')

	-- 基础算术
	_assert.assert_eq(2, 1 + 1, 'basic arithmetic')
	-- 字符串拼接
	_assert.assert_eq('hello', 'hel' .. 'lo', 'string concatenation')
	-- 布尔值
	_assert.assert_true(true, 'boolean true')
	_assert.assert_false(false, 'boolean false')

	-- Table 操作
	local t = { a = 1, b = 2 }
	_assert.assert_eq(1, t.a, 'table access')
	_assert.assert_eq(2, t.b, 'table access')

	print('test_lua_basic: OK')
end

-- 函数与闭包测试
local function _test_lua_functions()
	_assert.set_current_test('test_lua_functions')

	-- 函数定义与调用
	local function add(a, b)
		return a + b
	end
	_assert.assert_eq(5, add(2, 3), 'function call')

	-- 闭包测试
	local function make_counter()
		local count = 0
		return function()
			count = count + 1
			return count
		end
	end
	local counter = make_counter()
	_assert.assert_eq(1, counter(), 'closure first call')
	_assert.assert_eq(2, counter(), 'closure second call')

	print('test_lua_functions: OK')
end

-- 错误处理测试
local function _test_error_handling()
	_assert.set_current_test('test_error_handling')

	-- pcall 成功场景
	local ok, result = pcall(function() return 42 end)
	_assert.assert_true(ok, 'pcall success')
	_assert.assert_eq(42, result, 'pcall result')

	-- pcall 捕获错误场景
	local ok2, err = pcall(function() error('test error') end)
	_assert.assert_false(ok2, 'pcall catches error')
	_assert.assert_not_nil(err, 'pcall returns error message')

	print('test_error_handling: OK')
end

-- 执行所有测试
print('Running tests...')
print('')

_test_lua_basic()
_test_lua_functions()
_test_error_handling()

-- 执行模块测试
require('tests.test_display')
require('tests.test_input')

-- 打印摘要并返回退出码
_assert.print_summary()

local _, fail_count = _assert.get_stats()
return fail_count
