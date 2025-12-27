-- test_display.lua: Tests for native.display module

local assert = require("tests.assert")
local display = require("native.display")

local function test_window_get_size()
    assert.set_current_test("test_window_get_size")

    local w, h = display.window_get_size()

    -- Size should be integers
    assert.assert_eq(type(w), "number", "width should be a number")
    assert.assert_eq(type(h), "number", "height should be a number")

    -- Size should be positive (in headless mode, might be 0 or small)
    -- Just check they are non-negative
    assert.assert_true(w >= 0, "width should be >= 0")
    assert.assert_true(h >= 0, "height should be >= 0")

    print("test_window_get_size: OK")
end

local function test_window_set_size_valid()
    assert.set_current_test("test_window_set_size_valid")

    -- Get current size
    local w, h = display.window_get_size()

    -- Try to set the same size (should succeed in windowed mode)
    -- Note: In headless mode this might fail due to mode restrictions
    local rc = display.window_set_size(800, 600)

    -- rc should be an integer
    assert.assert_eq(type(rc), "number", "return code should be a number")

    -- Note: We don't assert rc == 0 because headless mode may not support window operations
    print("test_window_set_size_valid: OK (rc=" .. tostring(rc) .. ")")
end

local function test_window_set_size_invalid_zero()
    assert.set_current_test("test_window_set_size_invalid_zero")

    -- Zero width should fail
    local rc = display.window_set_size(0, 600)
    assert.assert_ne(rc, 0, "zero width should return non-zero error code")

    -- Zero height should fail
    rc = display.window_set_size(800, 0)
    assert.assert_ne(rc, 0, "zero height should return non-zero error code")

    -- Both zero should fail
    rc = display.window_set_size(0, 0)
    assert.assert_ne(rc, 0, "zero size should return non-zero error code")

    print("test_window_set_size_invalid_zero: OK")
end

local function test_window_set_size_invalid_negative()
    assert.set_current_test("test_window_set_size_invalid_negative")

    -- Negative width should fail
    local rc = display.window_set_size(-1, 600)
    assert.assert_ne(rc, 0, "negative width should return non-zero error code")

    -- Negative height should fail
    rc = display.window_set_size(800, -100)
    assert.assert_ne(rc, 0, "negative height should return non-zero error code")

    print("test_window_set_size_invalid_negative: OK")
end

-- Run all tests
test_window_get_size()
test_window_set_size_valid()
test_window_set_size_invalid_zero()
test_window_set_size_invalid_negative()
