# 11 - Spring Block Simulator

Hyperrealistic spring-mass block simulator with PBR rendering and an ImGui control panel. Foundational prototype for the LAGEngine physics constraint system and PBR material pipeline.

## Features

- **Spring-mass dynamics** with configurable stiffness, damping, and rest length
- **PBR rendering** (metallic-roughness workflow) with directional lighting
- **Interactive block dragging** with ray-cast picking (slab AABB method)
- **Orbit camera** (right-click orbit, middle-click pan, scroll zoom)
- **ImGui control panel** for adding/removing blocks and springs, tweaking physics and materials
- **Wireframe mode** toggle

## Dependencies

- **Vulkan SDK** (with shaderc for runtime GLSL compilation)
- **VulkanBase** shared library (from `../common/`)
- **GLFW** - windowing (via VulkanBase)
- **GLM** - math (via VulkanBase)
- **ImGui** - UI with Vulkan backend (via VulkanBase)

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

Or use the script:
```bash
./build_sim.sh Release
```

Run:
```bash
./build/SpringBlockSim
```

## Controls

| Input | Action |
|-------|--------|
| Right-click drag | Orbit camera |
| Middle-click drag | Pan camera |
| Scroll wheel | Zoom |
| Left click | Select block |
| Left drag on block | Move block on horizontal plane |

## Architecture

| File | Purpose |
|------|---------|
| `Physics.h/cpp` | Block/Spring structs, PhysicsWorld with sub-stepping integrator |
| `Renderer.h/cpp` | Vulkan PBR renderer with push constants and runtime shader compilation |
| `SpringMesh.h/cpp` | Procedural coil mesh generation for spring visualization |
| `Camera.h/cpp` | Orbit camera with projection/view matrix generation |
| `UI.h/cpp` | ImGui panels for scene editing and render config |
