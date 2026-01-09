-- tests/test_core.lua
-- desc: native_core 模块测试

local _assert = require('tests.assert')
local _core = require('native_core')

local function _test_quit_exists()
	_assert.set_current_test('test_quit_exists')
	_assert.assert_eq('function', type(_core.quit), 'quit should be a function')
	print('test_quit_exists: OK')
end

_test_quit_exists()
