# 04 - Particle System

Simple 3D particle system with collision detection, gravity, and path visualization.
Renders particles using Vulkan via the shared VulkanBase library, with color cycling
and variable lifespans.

## Features

- Gravity-driven particle emission
- Floor collision with friction and damping
- Color cycling through cyan, yellow, and magenta
- Variable particle lifespans
- Path trail visualization
- Runtime GLSL-to-SPIR-V compilation via shaderc

## Dependencies

- Vulkan SDK (with shaderc)
- GLFW (provided by VulkanBase)
- GLM (provided by VulkanBase)

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```
