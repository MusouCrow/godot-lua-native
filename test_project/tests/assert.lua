-- assert.lua: Minimal assertion library for testing

local M = {}

local test_count = 0
local fail_count = 0
local current_test = ""

function M.set_current_test(name)
    current_test = name
end

function M.assert_true(condition, message)
    test_count = test_count + 1
    if not condition then
        fail_count = fail_count + 1
        local msg = current_test .. ": FAIL"
        if message then
            msg = msg .. " - " .. message
        end
        print(msg)
        return false
    end
    return true
end

function M.assert_false(condition, message)
    return M.assert_true(not condition, message)
end

function M.assert_eq(expected, actual, message)
    test_count = test_count + 1
    if expected ~= actual then
        fail_count = fail_count + 1
        local msg = current_test .. ": FAIL - expected " .. tostring(expected) .. ", got " .. tostring(actual)
        if message then
            msg = msg .. " (" .. message .. ")"
        end
        print(msg)
        return false
    end
    return true
end

function M.assert_ne(expected, actual, message)
    test_count = test_count + 1
    if expected == actual then
        fail_count = fail_count + 1
        local msg = current_test .. ": FAIL - expected different from " .. tostring(expected)
        if message then
            msg = msg .. " (" .. message .. ")"
        end
        print(msg)
        return false
    end
    return true
end

function M.assert_nil(value, message)
    return M.assert_eq(nil, value, message)
end

function M.assert_not_nil(value, message)
    return M.assert_ne(nil, value, message)
end

-- Assert that a function raises an error
-- Returns true if error was raised, false otherwise
function M.assert_raises(func, expected_pattern, message)
    test_count = test_count + 1
    local ok, err = pcall(func)
    if ok then
        fail_count = fail_count + 1
        local msg = current_test .. ": FAIL - expected error but none raised"
        if message then
            msg = msg .. " (" .. message .. ")"
        end
        print(msg)
        return false
    end
    if expected_pattern then
        if not string.find(tostring(err), expected_pattern, 1, true) then
            fail_count = fail_count + 1
            local msg = current_test .. ": FAIL - error message '" .. tostring(err) .. "' does not match pattern '" .. expected_pattern .. "'"
            if message then
                msg = msg .. " (" .. message .. ")"
            end
            print(msg)
            return false
        end
    end
    return true
end

function M.get_stats()
    return test_count, fail_count
end

function M.reset_stats()
    test_count = 0
    fail_count = 0
end

function M.print_summary()
    print(string.format("\n=== Test Summary ==="))
    print(string.format("Total: %d, Passed: %d, Failed: %d", test_count, test_count - fail_count, fail_count))
    if fail_count == 0 then
        print("All tests passed!")
    else
        print("Some tests failed.")
    end
end

return M
