# godot-lua-native

A high-performance native Lua 5.5 integration for Godot 4.x via GDExtension.

**Features:**
- Native C++ performance with GDExtension
- Embedded Lua 5.5.0 interpreter
- Cross-platform support (Linux, macOS arm64, Windows)
- Direct Godot API bindings for Lua

---

<details>
<summary>中文文档 / Chinese Documentation</summary>

为 Godot 4.x 提供高性能原生 Lua 5.5 集成的 GDExtension 扩展。

**特性：**
- 基于 GDExtension 的原生 C++ 性能
- 内嵌 Lua 5.5.0 解释器
- 跨平台支持（Linux、macOS arm64、Windows）
- Lua 直接访问 Godot API

</details>

---

## Requirements / 系统要求

- Godot 4.x
- CMake 3.17+
- C++17 compatible compiler

## Installation / 安装

### From Release / 从发布版安装

1. Download the latest release
2. Copy `addons/luagd/` to your Godot project

### Build from Source / 从源码构建

```bash
git clone --recursive https://github.com/user/godot-lua-native.git
cd godot-lua-native
mkdir build && cd build
cmake ..
make
```

The compiled library will be placed in `../project/addons/luagd/`.

## Quick Start / 快速开始

### GDScript

```gdscript
extends Node

var lua_host

func _ready():
    lua_host = Engine.get_singleton("LuaHost")
    lua_host.run_file("res://scripts/main.lua")
```

### Lua

```lua
local core = require("native_core")
local input = require("native_input")

core.bind_update(function(delta)
    if input.is_pressed("jump") then
        print("Jump!")
    end
end)

core.bind_shutdown(function()
    print("Goodbye!")
end)
```

## Lua API

### native_core

| Function | Description |
|----------|-------------|
| `bind_update(func)` | Bind update callback, called every physics frame with `delta` |
| `bind_shutdown(func)` | Bind shutdown callback, called on game exit |

### native_input

| Function | Description |
|----------|-------------|
| `bind_input(func)` | Bind input callback `(action, pressed, strength, device_type)` |
| `is_pressed(action)` | Returns true on press frame |
| `is_hold(action)` | Returns true while held |
| `is_released(action)` | Returns true on release frame |
| `get_strength(action)` | Returns action strength (0.0~1.0) |
| `get_axis(neg, pos)` | Returns axis value (-1.0~1.0) |
| `get_vector(left, right, up, down)` | Returns normalized 2D vector (x, y) |
| `vibrate(weak, strong, duration)` | Trigger controller vibration |

### native_display

| Function | Description |
|----------|-------------|
| `window_get_size()` | Returns window width and height |
| `window_set_size(w, h)` | Set window size, returns 0 on success |

## Project Structure / 项目结构

```
godot-lua-native/
├── src/
│   ├── host/           # GDExtension entry and LuaHost singleton
│   ├── lua/            # Lua runtime wrapper
│   └── modules/        # Native modules (core, input, display, audio)
├── lua/                # Lua 5.5.0 source code
├── godot-cpp/          # Godot C++ bindings (submodule)
└── api/                # Lua API stubs for IDE support
```

## Development / 开发

### Run Tests / 运行测试

```bash
# From the repo root directory
godot --headless --path project -- tests/native/test_native_core.lua
```

### API Stubs / API 存根

The `api/` directory contains Lua type annotations for IDE autocompletion:
- `api/native_core.lua`
- `api/native_input.lua`
- `api/native_display.lua`

## License

[MIT License](LICENSE)
