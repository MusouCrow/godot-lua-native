#ifndef LUAGD_NODE_MODULE_H
#define LUAGD_NODE_MODULE_H

struct lua_State;

namespace godot {
class Node3D;
class ObjectID;
}

namespace luagd {

// 打开 native_node 模块。
// 提供基于 id 句柄的节点操作 API。
// 返回：在 Lua 栈上返回 1（模块表）。
int luaopen_native_node(lua_State *p_L);

// 清理节点模块资源。
// 释放所有节点引用。
void node_cleanup();

// 通过 native_node id 解析 Node3D。
// 仅供其他 native 模块内部使用，失败返回 nullptr。
godot::Node3D *node_resolve(godot::ObjectID p_id);

} // namespace luagd

#endif // LUAGD_NODE_MODULE_H
