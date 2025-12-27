extends SceneTree

# console.gd - Minimal test launcher for headless Lua testing.
# Usage: godot --headless --path test_project --script res://console.gd -- [path]
# If path is not provided, defaults to res://tests/main.lua

const DEFAULT_SCRIPT := "res://tests/main.lua"

func _init() -> void:
	var lua_host = Engine.get_singleton("LuaHost")
	if not lua_host:
		quit(1)
		return
	var path := _get_script_path()
	var exit_code: int = lua_host.run_file(path)
	quit(exit_code)

func _get_script_path() -> String:
	var args := OS.get_cmdline_user_args()
	return args[0] if args.size() > 0 else DEFAULT_SCRIPT
