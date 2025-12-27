# C++ 开发规范（src/**）

本规范用于约束本工程 `src/**` 下的所有 C++ 代码（包括头文件与实现文件）。其目标是：在“Godot 插件 + 调用 Godot API + 导出 C 风格函数到 Lua + 由 AI 产出代码”的背景下，做到风格统一、行为可预测、错误可读、可维护。

## 适用范围

- 适用：`src/**`
- 不适用（禁止按本规范重排/格式化/重构）：`godot-cpp/**`、`lua/**` 及其他第三方代码

## 语言与特性约束（硬规则）

- 标准：C++17
- 禁止异常：不使用 `throw` / `try` / `catch`
- 禁止 RTTI：不使用 `dynamic_cast` / `typeid`
- 禁止 STL：`src/**` 内禁止引入与使用任何 `std::*`（包括但不限于 `std::string`、`std::vector`、`std::unique_ptr`、`std::function`、`std::optional`、`std::move` 等）
- 禁止 `auto`（保持类型显式，便于审阅与绑定层稳定）
- 头文件不使用 `#pragma once`，统一使用 include guard

## Godot 类型与容器（替代 STL）

- 字符串：使用 `String` / `StringName`
  - 需要 UTF-8 字节串时：使用 `CharString`（通过 `String::utf8()` 等转换）
- 容器：使用 Godot 提供的容器（按实际可用类型选择）
  - 例如：`Vector<T>`、`List<T>`、`HashMap<K, V>`、`HashSet<T>`
- 引用计数对象：使用 `Ref<T>`
- 句柄：Lua 暴露的对象句柄统一为 `uint64_t`，语义为 `ObjectID/instance_id`

## 命名规则（硬规则）

- 类型/类/结构体/枚举：`PascalCase`
- 函数/方法/文件内静态函数：`snake_case`
- 常量/枚举值：`CONSTANT_CASE`
- 成员变量：`snake_case`，不加 `m_`/`_` 前缀
- 函数参数：统一加 `p_` 前缀（避免与成员同名）
  - 例：`void set_size(int32_t p_width, int32_t p_height);`

## 文件命名（硬规则）

- 文件名使用 `snake_case`，并力求**全局可辨识**：即使脱离目录也能看出职责，避免 `common.*`、`utils.*`、`helper.*` 等泛化命名。
- 文件名应与其所在目录职责相关联（目录提供“作用域”，文件名提供“领域/对象”）。
- `src/modules/**` 下的模块文件命名统一为：`<domain>_module.{h,cpp}`
  - 例：`display_module.h` / `display_module.cpp`、`input_module.h` / `input_module.cpp`
- 若某个领域需要拆分多个实现文件，使用清晰后缀：`<domain>_module_<topic>.cpp`（例如 `<topic>` 为 `window`、`cursor` 等），禁止使用无意义后缀 `2`、`new` 等。

## 格式与排版（不依赖 clang-format）

> 本项目不强制 `clang-format`，因此以下规则必须人工遵守，确保 AI 输出一致。

- 缩进：使用 tab（不要用空格缩进）
- 指针/引用写法：`Type *p`、`Type &r`（`*`/`&` 贴变量名）
- include 顺序（每组之间空一行）：
  1) `#include "this_file.h"`
  2) Godot / 本项目其它头
  3) 第三方头（例如 Lua C API 头）
- 单文件职责：避免“巨无霸文件”；模块/子系统拆分到独立 `.h/.cpp`
- 可读性优先：长条件拆行、早返回、避免深层嵌套

## 线程模型（硬规则）

- **任何触碰 Godot API 的代码必须在主线程同步执行**
- 每个导出给 Lua 的入口函数（或模块入口）应在最开始调用统一的“主线程检查”辅助（具体函数名以实现为准），失败时直接报错

## Lua 绑定层专项规范（硬规则）

### 公共 ABI 类型

Lua ↔ C++ 的公共 ABI 仅允许：
- `nil`
- `bool`
- `int`（建议：Lua 整数）
- `number`（Lua 浮点）
- `string`
- `handle(uint64_t)`：`instance_id`

禁止：
- 任何 Godot 结构体直接映射到 Lua（如 `Vector2/Vector3/Transform/...`）
- Lua 侧保存 C++ 指针（不使用 `lightuserdata` 作为对象句柄）
- 以 Lua table 作为跨模块通用数据结构（如确有需要，必须单独设计并在文档中写明）

### handle 校验（统一模板）

任何接收 `handle` 的函数必须遵循以下步骤：
1) `handle != 0`，否则报错
2) 通过 `ObjectDB`（或等价机制）查找对象
3) 对象不存在则报错
4) 如需要特定类型（Node/Window/...），必须显式校验类型，不允许静默失败

## 所有权与销毁（显式管理，硬规则）

- Lua 不依赖 GC 管理对象生命周期；必须显式销毁
- C++ 侧维护 `owned` 记录（例如 `OwnedRegistry`）：
  - 仅允许销毁 owned 对象；尝试销毁非 owned 对象必须报错 `not owned`
  - `Node` 的销毁一律使用 `queue_free()`（释放时机交给 Godot）
  - 非 `Node` 对象按 Godot 约定释放，并在实现处写清楚策略
  - 对象销毁通知到来时自动清理 owned 记录，避免悬挂状态

## 修改规则（AI 协作约束）

- 不做“顺手重构”：任何提交应围绕当前任务，避免无关格式改动
- 不改第三方目录：禁止触碰 `godot-cpp/**`、`lua/**` 的风格与结构（除非任务明确要求）
- 任何新增模块/文件必须在提交描述中说明其职责与边界
