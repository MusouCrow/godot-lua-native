extends SceneTree

# console.gd - Minimal test launcher for headless Lua testing.
# Usage: godot --headless --path test_project --script res://console.gd

func _init() -> void:
	var lua_host = Engine.get_singleton("LuaHost")
	var exit_code: int = lua_host.run_file("res://tests/main.lua") if lua_host else 1
	quit(exit_code)
