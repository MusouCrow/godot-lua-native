-- path: tests/test_system.lua
-- desc: native_system 模块测试

local _assert = require('tests.assert')
local _system = require('native_system')

-- Local Cache
local type = type
local print = print

-- 测试获取操作系统名称
local function _test_get_name()
	_assert.set_current_test('test_get_name')

	local name = _system.get_name()

	-- 返回值应为字符串类型
	_assert.assert_eq(type(name), 'string', 'get_name should return a string')

	-- 返回值应非空
	_assert.assert_true(#name > 0, 'get_name should return a non-empty string')

	-- 验证返回值是已知的操作系统名称之一
	local valid_names = {
		Windows = true,
		macOS = true,
		Linux = true,
		Android = true,
		iOS = true,
		Web = true,
		Unknown = true,
	}
	_assert.assert_true(valid_names[name] ~= nil, 'get_name should return a known OS name, got: ' .. name)

	print('test_get_name: OK (name=' .. name .. ')')
end

-- 执行所有测试
_test_get_name()
