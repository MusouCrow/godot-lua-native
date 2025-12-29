-- path: tests/assert.lua
-- desc: 最小断言库，用于测试用例的断言和统计

-- Local Cache: 高频使用的外部函数
local tostring = tostring
local print = print
local pcall = pcall
local string_find = string.find
local string_format = string.format

---@class tests.assert
local _assert = {}

-- 私有状态
local _test_count = 0
local _fail_count = 0
local _current_test = ''

---设置当前测试名称，用于错误输出上下文
---@param name string 测试名称
function _assert.set_current_test(name)
	_current_test = name
end

---断言条件为真
---@param condition boolean 条件
---@param message? string 失败时的提示信息
---@return boolean success 是否成功
function _assert.assert_true(condition, message)
	_test_count = _test_count + 1
	if not condition then
		_fail_count = _fail_count + 1
		local msg = _current_test .. ': FAIL'
		if message then
			msg = msg .. ' - ' .. message
		end
		print(msg)
		return false
	end
	return true
end

---断言条件为假
---@param condition boolean 条件
---@param message? string 失败时的提示信息
---@return boolean success 是否成功
function _assert.assert_false(condition, message)
	return _assert.assert_true(not condition, message)
end

---断言两值相等
---@param expected any 期望值
---@param actual any 实际值
---@param message? string 失败时的提示信息
---@return boolean success 是否成功
function _assert.assert_eq(expected, actual, message)
	_test_count = _test_count + 1
	if expected ~= actual then
		_fail_count = _fail_count + 1
		local msg = _current_test .. ': FAIL - expected ' .. tostring(expected) .. ', got ' .. tostring(actual)
		if message then
			msg = msg .. ' (' .. message .. ')'
		end
		print(msg)
		return false
	end
	return true
end

---断言两值不相等
---@param expected any 不期望的值
---@param actual any 实际值
---@param message? string 失败时的提示信息
---@return boolean success 是否成功
function _assert.assert_ne(expected, actual, message)
	_test_count = _test_count + 1
	if expected == actual then
		_fail_count = _fail_count + 1
		local msg = _current_test .. ': FAIL - expected different from ' .. tostring(expected)
		if message then
			msg = msg .. ' (' .. message .. ')'
		end
		print(msg)
		return false
	end
	return true
end

---断言值为 nil
---@param value any 待检查的值
---@param message? string 失败时的提示信息
---@return boolean success 是否成功
function _assert.assert_nil(value, message)
	return _assert.assert_eq(nil, value, message)
end

---断言值不为 nil
---@param value any 待检查的值
---@param message? string 失败时的提示信息
---@return boolean success 是否成功
function _assert.assert_not_nil(value, message)
	return _assert.assert_ne(nil, value, message)
end

---断言函数抛出错误
---@param func function 待执行的函数
---@param expected_pattern? string 期望的错误消息模式（plain match）
---@param message? string 失败时的提示信息
---@return boolean success 是否成功
function _assert.assert_raises(func, expected_pattern, message)
	_test_count = _test_count + 1
	local ok, err = pcall(func)
	if ok then
		_fail_count = _fail_count + 1
		local msg = _current_test .. ': FAIL - expected error but none raised'
		if message then
			msg = msg .. ' (' .. message .. ')'
		end
		print(msg)
		return false
	end
	if expected_pattern then
		if not string_find(tostring(err), expected_pattern, 1, true) then
			_fail_count = _fail_count + 1
			local msg = _current_test ..
			': FAIL - error message \'' .. tostring(err) .. '\' does not match pattern \'' .. expected_pattern .. '\''
			if message then
				msg = msg .. ' (' .. message .. ')'
			end
			print(msg)
			return false
		end
	end
	return true
end

---获取测试统计
---@return number test_count 总测试数
---@return number fail_count 失败数
function _assert.get_stats()
	return _test_count, _fail_count
end

---重置测试统计
function _assert.reset_stats()
	_test_count = 0
	_fail_count = 0
end

---打印测试摘要
function _assert.print_summary()
	print(string_format('\n=== Test Summary ==='))
	print(string_format('Total: %d, Passed: %d, Failed: %d', _test_count, _test_count - _fail_count, _fail_count))
	if _fail_count == 0 then
		print('All tests passed!')
	else
		print('Some tests failed.')
	end
end

return _assert
