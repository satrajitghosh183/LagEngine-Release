# 02 - Spring Cloth Simulation

Spring-mass cloth and flag simulation with interactive parameter tweaking.

## Features

- Hook force and brake force model for cloth dynamics
- Octree-based self-collision detection
- Sphere collision obstacles
- Real-time parameter adjustment (stiffness, damping, wind) via ImGui
- Vulkan rendering with GLFW windowing (via shared VulkanBase library)
- Wireframe / solid toggle

## Dependencies

- Vulkan SDK (with shaderc for runtime GLSL-to-SPIR-V compilation)
- GLFW (provided transitively by VulkanBase)
- GLM (provided transitively by VulkanBase)
- ImGui with Vulkan backend (provided transitively by VulkanBase)

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## Controls

- **Left-click drag**: Orbit camera
- **Scroll wheel**: Zoom
- **Space**: Toggle wireframe
- **Arrow keys**: Move first sphere obstacle
- **Numpad +/-**: Adjust wind velocity
- **Escape**: Quit

## Language

C++17
