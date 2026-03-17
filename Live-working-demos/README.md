# LAGEngine - Foundational Demos

A collection of standalone physics, rendering, and tooling demos built during the development of [LAGEngine](https://github.com/satrajitghosh183/LAGEngine). Each demo explores a different technique that was later integrated into the full engine.

## Demos

| Directory | Description | Tech |
| --------- | ----------- | ---- |
| [01-verlet-physics](01-verlet-physics/) | 2D/3D cloth simulation with Verlet integration, cannon projectiles, rigid body tests | C++17, SFML, OpenGL, GLFW |
| [02-spring-cloth-simulation](02-spring-cloth-simulation/) | Spring-mass flag/cloth simulation with interactive parameter tweaking | C++11, SDL, OpenGL, GLEW |
| [03-xpbd-soft-body](03-xpbd-soft-body/) | XPBD soft body physics with PBR materials and shadow rendering | C++17, OpenGL, GLFW, Eigen |
| [04-particle-system](04-particle-system/) | 3D particle system with collision, gravity, and path visualization | C++, freeglut, OpenGL |
| [05-gpu-particles-cuda](05-gpu-particles-cuda/) | CUDA-accelerated GPU particle simulation with RL optimization | C++17, CUDA, OpenGL, Python |
| [06-deferred-rendering](06-deferred-rendering/) | Deferred rendering pipeline with render graph DAG scheduling | C++17, CUDA, OpenGL, GLFW |
| [07-robot-arm-kinematics](07-robot-arm-kinematics/) | Robot arm FK/IK simulation with DH parameters and PID control | C++17, OpenGL, Eigen, ImGuizmo |
| [08-imgui-editor](08-imgui-editor/) | ImGui-based scene editor prototype with panels and theming | C++17, OpenGL, GLFW, ImGui |
| [09-cmake-shader-tools](09-cmake-shader-tools/) | Python tools for CMake generation and AI-powered GLSL shader authoring | Python 3.8+, Ollama |
| [10-neural-character-generation](10-neural-character-generation/) | Neural character generation pipeline (work in progress) | Python, PyTorch |
| [11-spring-block-sim](11-spring-block-sim/) | PBR spring-block simulator with bloom, shadow mapping, and ImGui controls | C++17, OpenGL, GLFW, ImGui |

## Building

Each demo is self-contained with its own `CMakeLists.txt` (or Python setup for 09). See the README inside each folder for specific dependencies and build instructions.

General pattern for C++ demos:

```bash
cd <demo-folder>
mkdir build && cd build
cmake ..
cmake --build .
```

## Requirements

Most demos need a subset of:

- C++17 compiler (GCC 7+, Clang 5+, MSVC 19.14+)
- CMake 3.16+
- OpenGL 3.3+ development libraries
- Platform-specific deps listed in each demo's README

## License

See [LICENSE](LICENSE) for details.
