## godot-lua-native

一个基于 Godot 4.x GDExtension 的 Lua 5.5 接入实验工程：在 C++ 层按需实现导出给 Lua 的 C 风格接口，Lua 侧以同步、命令式方式驱动 Godot。

## 文档范围

- 本 README：面向“如何构建与运行测试”的快速上手说明。
- `src/README.md`：面向实现者的开发范式与约束（模块划分、句柄/所有权、线程/错误模型等）。
- `src/cpp_guidelines.md`：`src/**` 下的 C++ 代码规范（禁用 STL/异常/RTTI 等）。
- `proposal.md`：文件级执行文档（模块落地步骤、测试工程约定等）。

## 目录结构（当前）

- `src/`：扩展源码（GDExtension 入口、Lua runtime、线程检查等）
- `lua/`：Lua 5.5 源码（作为静态库编译并链接进扩展）
- `godot-cpp/`：Godot C++ 绑定
- `test_project/`：最小 Godot 测试工程（headless 运行 Lua 用例）
  - `test_project/addons/luagd/`：扩展产物与 `luagd.gdextension`
  - `test_project/console.gd`：启动器（仅桥接启动与退出码）
  - `test_project/tests/*.lua`：Lua 测试用例

## 构建（CMake）

依赖：
- CMake ≥ 3.17
- 支持 C++17 的编译器
- `godot` 命令可用（用于运行 `test_project` 的集成测试）

构建（默认 `template_debug`）：
- `cmake -S . -B build -DGODOTCPP_TARGET=template_debug`
- `cmake --build build -j`

产物输出：
- 扩展动态库会直接输出到 `test_project/addons/luagd/`
- `test_project/addons/luagd/luagd.gdextension` 已按平台/架构枚举动态库文件名（例如 macOS `arm64`）

## 运行集成测试（依赖 Godot）

在仓库根目录执行：
- `godot --headless --path test_project --script res://console.gd`
- 指定测试入口脚本（位置参数，仅取第一个 user arg）：
  - `godot --headless --path test_project --script res://console.gd -- res://tests/test_display.lua`

约定：
- 未提供脚本参数时默认执行：`res://tests/main.lua`
- `LuaHost.run_file(path)` 返回退出码：`0` 通过，非 `0` 失败
- 失败信息通过 Godot 的 `printerr` 输出到命令行/日志

## Lua 模块命名空间

Lua 侧模块统一使用 `native.*` 前缀（例如 `require("native.display")`）。

## License

MIT, see `LICENSE`.
