-- main.lua: Test entry point
-- Executes all test files and returns exit code (0 = success, non-zero = failure)

local assert = require("tests.assert")

print("=== Lua Native Test Suite ===")
print("")

-- Run basic sanity tests
local function test_lua_basic()
	assert.set_current_test("test_lua_basic")

	-- Test basic Lua functionality
	assert.assert_eq(2, 1 + 1, "basic arithmetic")
	assert.assert_eq("hello", "hel" .. "lo", "string concatenation")
	assert.assert_true(true, "boolean true")
	assert.assert_false(false, "boolean false")

	-- Test table operations
	local t = { a = 1, b = 2 }
	assert.assert_eq(1, t.a, "table access")
	assert.assert_eq(2, t.b, "table access")

	print("test_lua_basic: OK")
end

local function test_lua_functions()
	assert.set_current_test("test_lua_functions")

	-- Test function definition and call
	local function add(a, b)
		return a + b
	end
	assert.assert_eq(5, add(2, 3), "function call")

	-- Test closures
	local function make_counter()
		local count = 0
		return function()
			count = count + 1
			return count
		end
	end
	local counter = make_counter()
	assert.assert_eq(1, counter(), "closure first call")
	assert.assert_eq(2, counter(), "closure second call")

	print("test_lua_functions: OK")
end

local function test_error_handling()
	assert.set_current_test("test_error_handling")

	-- Test pcall with success
	local ok, result = pcall(function() return 42 end)
	assert.assert_true(ok, "pcall success")
	assert.assert_eq(42, result, "pcall result")

	-- Test pcall with error
	local ok2, err = pcall(function() error("test error") end)
	assert.assert_false(ok2, "pcall catches error")
	assert.assert_not_nil(err, "pcall returns error message")

	print("test_error_handling: OK")
end

-- Run all tests
print("Running tests...")
print("")

test_lua_basic()
test_lua_functions()
test_error_handling()

-- Print summary and return exit code
assert.print_summary()

local _, fail_count = assert.get_stats()
return fail_count
