# 11 - Spring Block Simulator

Hyperrealistic spring-mass block simulator with PBR rendering, bloom, shadow mapping, and an ImGui control panel. Foundational prototype for the LAGEngine physics constraint system and PBR material pipeline.

## Features

- **Spring-mass dynamics** with configurable stiffness, damping, and rest length
- **PBR rendering** (metallic-roughness workflow) with HDR lighting
- **Shadow mapping** (2048x2048 directional shadow map)
- **Bloom post-processing** with configurable threshold and strength
- **Tone mapping** (HDR to LDR exposure control)
- **Interactive block dragging** with ray-cast picking (slab AABB method)
- **Orbit camera** (right-click orbit, middle-click pan, scroll zoom)
- **ImGui control panel** for adding/removing blocks and springs, tweaking physics and materials

## Dependencies

All dependencies are fetched automatically via CMake FetchContent:
- **GLFW 3.4** - windowing
- **GLM 1.0.1** - math
- **ImGui 1.90.1** - UI
- **GLAD** - OpenGL loader (vendored in `external/glad/`)
- **OpenGL 3.3+** - rendering

## Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

Run from the build directory (shaders are copied automatically):
```bash
./SpringBlockSim
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
| `Renderer.h/cpp` | Multi-pass PBR renderer (shadow, color, bloom, tonemap) |
| `SpringMesh.h/cpp` | Procedural coil mesh generation for spring visualization |
| `Camera.h/cpp` | Orbit camera with projection/view matrix generation |
| `Shader.h/cpp` | GLSL shader loader with uniform helpers |
| `UI.h/cpp` | ImGui panels for scene editing and render config |
| `shaders/` | PBR, shadow, bloom, ground, and tonemap GLSL shaders |
