# LAG Engine

LAG Engine is a modular, cross-platform C++17 game engine built from scratch for
research, simulation, and game development.

## Highlights

- **Vulkan rendering** with a deferred PBR pipeline, SSAO, IBL, shadows, and compute-based post-processing
- **Multi-physics**: rigid body (sequential impulse), XPBD soft body, Verlet cloth, SPH fluids
- **Scripting**: Lua 5.4 with hot-reload *and* LAGScript (GDScript-like) with its own lexer/parser/interpreter
- **Visual scripting**: graph-based VM with 80+ nodes
- **Networking**: reliable UDP (ENet-style), WebSocket (RFC 6455), RPC system, server-authoritative replication
- **Animation**: state machines, blend trees (1D/2D/additive), skeletal IK (two-bone, CCD, FABRIK)
- **Custom UI**: Control-node tree with anchors, themes, layouts, and a dock system
- **Asset pipeline**: GUID-based registry, `.lagscene` text format with resource refs and sub-resources
- **CUDA integration** with Vulkan interop via `VK_KHR_external_memory`
- **Work-stealing job system** with DAG task graph
- **ImGui-based editor** with 13+ panels (scene hierarchy, viewport, material editor, profiler, …)

## Platforms

Windows, Linux, macOS (Vulkan on all three via GLFW).

## Quick Links

- [Building from source](./getting-started/building.md)
- [Your first scene](./getting-started/first-scene.md)
- [API reference](./reference/api.md)
