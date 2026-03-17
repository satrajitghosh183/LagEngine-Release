# LAG Engine

**LAG Engine** is a modular, cross-platform 3D game engine written in C++17. It provides a complete runtime with physics simulation, deferred rendering, Lua scripting, spatial audio, and a full-featured editor built on ImGui.

Designed for both educational use and as a foundation for real-time applications, the engine ships with ten example projects covering rendering, rigid/soft body physics, fluid dynamics, character control, and robotic simulation. It also includes an RL benchmark suite for frame scheduling optimization and eleven foundational demos from early engine development.

---

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Building from Source](#building-from-source)
- [Running](#running)
- [Project Structure](#project-structure)
- [CMake Options](#cmake-options)
- [Editor](#editor)
- [Scripting](#scripting)
- [Documentation](#documentation)
- [RL Backend](#rl-backend)
- [Live Working Demos](#live-working-demos)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

---

## Features

### Graphics

- OpenGL 4.x deferred rendering pipeline with GBuffer
- Physically-based rendering (PBR) material system
- Shadow mapping, screen-space ambient occlusion (SSAO), and image-based lighting (IBL)
- Frustum culling and shader/material draw-call batching
- 2D batch renderer and GPU-instanced particle system
- Shader hot-reload from disk

### Physics

- Rigid body dynamics with sequential-impulse constraint solver
- Collision detection with spatial hash broadphase (sphere, box, capsule, plane)
- Soft body simulation via XPBD solver with tearable cloth
- SPH fluid dynamics with spatial hash acceleration
- Constraint system: fixed, hinge, and slider joints
- Character controller with slope handling and jump mechanics
- Multiple integrators: Euler, Verlet, semi-implicit

### Audio

- 3D spatial audio via OpenAL Soft
- Distance attenuation and Doppler effect
- Streaming playback with looping support

### Scene and Entity System

- Entity-component architecture with UUID-based entity handles
- Parent-child transform hierarchy with cached world matrices
- JSON scene serialization and deserialization
- Scene merging and prefab support

### Scripting

- Lua integration with C++ bindings for entities, components, math, input, and audio
- Script hot-reload
- Interactive scripting console in the editor

### Editor

- Full docking-based editor (ImGui) with play/pause/step controls
- Scene hierarchy, viewport, component inspector, asset browser, console, and profiler panels
- Undo/redo command system
- Gizmo-based entity manipulation
- Shader hot-reload via file watcher
- Theme editor and auto-save

### Platform

- Cross-platform: Windows, macOS, Linux
- GLFW window and input abstraction
- File dialogs and file watcher for asset hot-reload
- Multithreaded job system with work-stealing scheduler

---

## Architecture

```text
Application (main loop, fixed timestep)
  |
  +-- Window (GLFW)
  +-- SceneManager
  |     +-- Scene -> Entity/Component storage
  |     +-- PhysicsServer -> PhysicsWorld, CollisionDetector, Constraints
  +-- Renderer3D -> GBuffer, ShadowMap, SSAO, IBL, DeferredRenderer
  +-- AudioEngine (OpenAL)
  +-- ScriptEngine (Lua)
  +-- JobSystem (work-stealing thread pool)
```

The update loop uses a fixed timestep for physics (default 60 Hz) with variable-rate rendering. The renderer collects draw commands via `Submit()`, sorts by shader and material, and flushes in `EndScene()`. Physics runs broadphase spatial hashing, narrowphase collision detection, and constraint solving each fixed step.

For a detailed breakdown, see [Documentation/Architecture/EngineOverview.md](Documentation/Architecture/EngineOverview.md).

---

## Prerequisites

### Windows

- Visual Studio 2019 or later with the **C++ Desktop Development** workload
- CMake 3.20 or later

### macOS

```bash
brew install cmake llvm openal-soft pkg-config
```

### Linux (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake git \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libopenal-dev
```

---

## Quick Start

The included `setup.sh` script detects your OS, installs dependencies, builds the engine, and provides an interactive terminal menu for running demos.

```bash
git clone git@github.com:satrajitghosh183/LAGEngine.git
cd LAGEngine
bash setup.sh          # Linux / macOS / WSL / Git Bash
# or on Windows CMD/PowerShell:
setup.bat
```

CLI flags for non-interactive use:

```bash
bash setup.sh --install       # Install system dependencies only
bash setup.sh --build-engine  # Build the main engine
bash setup.sh --build-rl      # Build RL_Backend
bash setup.sh --build-all     # Build everything (engine + RL + demos)
```

## Building from Source

If you prefer to build manually:

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

On Windows with Visual Studio:

```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Alternatively, open the project folder directly in Visual Studio or CLion and use the built-in CMake integration.

---

## Running

All executables are output to `build/bin/`.

| Executable | Description |
| --- | --- |
| `GameEngineEditor` | Full editor application |
| `HelloTriangle` | Minimal rendering example |
| `PhysicsDemo` | Rigid body physics simulation |
| `ClothSimulation` | Tearable cloth with wind interaction |
| `CharacterController` | Player movement and jumping |
| `FluidSimulation` | SPH fluid dynamics |
| `RobotArm` | Articulated robotic arm with IK |
| `Physics2DDemo` | 2D Verlet cloth and cannon physics |
| `HXPBDPhysics` | XPBD soft body simulation |
| `FlagSimulation` | Spring-mass flag physics |
| `LegacyParticles` | Classic particle system |

```bash
./bin/GameEngineEditor
./bin/PhysicsDemo
```

---

## Project Structure

```text
LAGEngine/
    Assets/                  Game assets (shaders, textures, scenes)
    Content/                 Presets and project templates
    Documentation/
        API/                 API reference (Core, Scene)
        Architecture/        Engine architecture overview
        Manual/              Getting started, editor guide, scripting API
        Tutorials/           Step-by-step tutorials (5 topics)
    Editor/
        Core/                Editor state, command system, hotkeys
        Gizmos/              Viewport gizmos
        Panels/              All editor panels (13+)
    Engine/
        Animation/           Skeletal animation and state machine
        Assets/              Asset management and caching
        Audio/               OpenAL audio engine
        Core/                Application, logger, job system, frame scheduler, UUID
        Graphics/            Renderer, shaders, materials, meshes, deferred pipeline
        ParticleSystem/      GPU particle compute and instanced rendering
        Physics/
            Character/       Character controller
            Collision/       Collision detection, spatial hash
            Constraints/     Fixed, hinge, slider joints
            Fluids/          SPH solver, spatial hash grid, kernels
            Integrators/     Euler, Verlet, semi-implicit
            SoftBody/        XPBD solver, tearable cloth, octree accelerator
        Platform/            Window, input, file dialogs, file watcher
        Robotics/            Robotic arm simulation, IK solver
        Scene/               Entity, scene, components, serialization, prefabs
        Scripting/           Lua engine, script bindings
        UI/                  UI renderer
        Utilities/           Debug draw, math utilities
    Examples/                10 example applications
        01_HelloTriangle/
        02_PhysicsDemo/
        03_ClothSimulation/
        04_CharacterController/
        05_FluidSimulation/
        06_RobotArm/
        07_Physics2DDemo/
        08_HXPBDPhysics/
        09_FlagSimulation/
        10_LegacyParticles/
    External/                Third-party libraries (bundled)
    Tests/                   Unit test suite
    Tools/                   Utility scripts
    RL_Backend/                 Multithreaded rendering + RL benchmark suite
        engine/              C++20 job system, indirect draw, physics
        demos/               6 visual + headless demos
        rl/                  Python RL training (12 algorithms)
        bindings/            pybind11 environment bridge
    Live-working-demos/      11 foundational physics/rendering prototypes
        01-verlet-physics/
        02-spring-cloth-simulation/
        03-xpbd-soft-body/
        04-particle-system/
        05-gpu-particles-cuda/
        06-deferred-rendering/
        07-robot-arm-kinematics/
        08-imgui-editor/
        09-cmake-shader-tools/
        10-neural-character-generation/
        11-spring-block-sim/
```

---

## CMake Options

| Option | Default | Description |
| --- | --- | --- |
| `BUILD_EXAMPLES` | `ON` | Build the example applications |
| `BUILD_EDITOR` | `ON` | Build the editor |
| `BUILD_TESTS` | `ON` | Build the unit test suite |
| `BUILD_SHARED_LIBS` | `OFF` | Build as shared libraries instead of static |

```bash
cmake .. -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF
```

---

## Editor

The editor provides a complete scene authoring environment:

- **Scene Hierarchy** -- tree view of all entities with drag-and-drop reparenting
- **Viewport** -- 3D scene view with translation/rotation/scale gizmos
- **Components Inspector** -- edit transform, mesh, material, physics, audio, and script properties
- **Asset Browser** -- browse and import project assets
- **Console** -- filterable log output from the engine logger
- **Scripting Console** -- interactive Lua REPL with full engine API access
- **Profiler** -- per-frame timing breakdown
- **Graphics Settings** -- toggle VSync, SSAO, shadow quality, and other render options
- **Theme Editor** -- customize the editor appearance

Play mode snapshots the scene state, runs the simulation, and restores on stop. Undo/redo is supported for entity and component operations.

---

## Scripting

Lua scripts attach to entities via `ScriptComponent`. The engine exposes bindings for:

- Entity creation, destruction, and queries
- Component access (Transform, RigidBody, Audio, etc.)
- Math types (Vec3, Quaternion, Mat4)
- Input polling (keyboard, mouse)
- Audio playback
- Logging

Scripts are hot-reloaded when the source file changes on disk.

See [Documentation/Manual/ScriptingAPI.md](Documentation/Manual/ScriptingAPI.md) for the full API reference.

---

## Documentation

| Document | Path |
| --- | --- |
| Getting Started | [Documentation/Manual/GettingStarted.md](Documentation/Manual/GettingStarted.md) |
| Editor Guide | [Documentation/Manual/EditorGuide.md](Documentation/Manual/EditorGuide.md) |
| Scripting API | [Documentation/Manual/ScriptingAPI.md](Documentation/Manual/ScriptingAPI.md) |
| Engine Architecture | [Documentation/Architecture/EngineOverview.md](Documentation/Architecture/EngineOverview.md) |
| Core API Reference | [Documentation/API/CoreAPI.md](Documentation/API/CoreAPI.md) |
| Scene API Reference | [Documentation/API/SceneAPI.md](Documentation/API/SceneAPI.md) |
| Tutorials (5) | [Documentation/Tutorials/](Documentation/Tutorials/) |

---

## RL Backend

`RL_Backend/` is a standalone multithreaded rendering and reinforcement learning benchmark suite. It explores how RL agents can optimize real-time frame scheduling decisions in a rendering pipeline.

**Key components:**

- **C++20 engine** with a Chase-Lev work-stealing job system, indirect draw rendering, and N-body physics
- **6 demo executables**: hello triangle, 10K–100K instanced cubes, single-threaded and parallel N-body, RL-driven physics, and a headless job system microbenchmark
- **Python RL training** with 12 algorithms (PPO, SAC, DQN, A2C, TD3, AWSS, and more)
- **pybind11 bindings** exposing a Gymnasium-compatible `FrameSchedulerEnv`

Build and run via `bash setup.sh` (option 4 from the menu), or manually:

```bash
cd RL_Backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)

# Run a demo
./bin/demo_physics_parallel

# Train an RL agent
cd ../rl
python train.py --algo ppo --env FrameSchedulerEnv --steps 50000
```

See [RL_Backend/README.md](RL_Backend/README.md) for full details.

---

## Live Working Demos

`Live-working-demos/` contains 11 self-contained projects built during early engine development. Each explores a specific technique that was later integrated into the main engine.

| Demo | Description | Tech |
| --- | --- | --- |
| [01-verlet-physics](Live-working-demos/01-verlet-physics/) | 2D/3D cloth simulation with Verlet integration | C++17, SFML, OpenGL, GLFW |
| [02-spring-cloth-simulation](Live-working-demos/02-spring-cloth-simulation/) | Spring-mass flag/cloth simulation | C++11, SDL, OpenGL, GLEW |
| [03-xpbd-soft-body](Live-working-demos/03-xpbd-soft-body/) | XPBD soft body physics with PBR materials | C++17, OpenGL, GLFW, Eigen |
| [04-particle-system](Live-working-demos/04-particle-system/) | 3D particle system with collision and gravity | C++, freeglut, OpenGL |
| [05-gpu-particles-cuda](Live-working-demos/05-gpu-particles-cuda/) | CUDA-accelerated GPU particle simulation with RL | C++17, CUDA, OpenGL |
| [06-deferred-rendering](Live-working-demos/06-deferred-rendering/) | Deferred rendering pipeline with DAG scheduling | C++17, CUDA, OpenGL, GLFW |
| [07-robot-arm-kinematics](Live-working-demos/07-robot-arm-kinematics/) | Robot arm FK/IK with DH parameters and PID control | C++17, OpenGL, Eigen, ImGuizmo |
| [08-imgui-editor](Live-working-demos/08-imgui-editor/) | ImGui-based scene editor prototype | C++17, OpenGL, GLFW, ImGui |
| [09-cmake-shader-tools](Live-working-demos/09-cmake-shader-tools/) | Python tools for CMake generation and AI shader authoring | Python 3.8+, Ollama |
| [10-neural-character-generation](Live-working-demos/10-neural-character-generation/) | Neural character generation pipeline (WIP) | Python, PyTorch |
| [11-spring-block-sim](Live-working-demos/11-spring-block-sim/) | PBR spring-block simulator with bloom and shadow mapping | C++17, OpenGL, GLFW, ImGui |

Each C++ demo has its own `CMakeLists.txt` and README (demos 09-10 are Python-only). Build any C++ demo with:

```bash
cd Live-working-demos/<demo-folder>
mkdir build && cd build
cmake ..
cmake --build .
```

Or build all demos at once via `bash setup.sh` (option 6 from the menu).

---

## Troubleshooting

### macOS OpenGL Deprecation Warnings

macOS has deprecated OpenGL but it remains functional. Suppress warnings with:

```bash
cmake .. -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations"
```

### Linux Missing OpenGL Headers

```bash
sudo apt install libgl1-mesa-dev
```

### Windows CMake Cannot Find Compiler

Ensure the Visual Studio C++ Desktop Development workload is installed. You can also specify the generator explicitly:

```cmd
cmake -G "Visual Studio 17 2022" ..
```

### Black Viewport in the Editor

Verify that a camera entity exists in the scene with the Main Camera flag enabled, and that its transform is positioned to view scene content.

### Missing Shaders at Runtime

When running from a build directory, ensure `Assets/Shaders/` is accessible. The engine resolves shader paths relative to the executable and project root via `RuntimePaths`.

---

## Contributing

Contributions are welcome! LAG Engine is open source under the MIT License and we appreciate help from the community.

### How to Contribute

1. **Fork** the repository on GitHub
2. **Clone** your fork:

   ```bash
   git clone https://github.com/<your-username>/LAGEngine.git
   cd LAGEngine
   ```

3. **Create a branch** for your change:

   ```bash
   git checkout -b feature/your-feature-name
   ```

4. **Build and test** to make sure everything works:

   ```bash
   bash setup.sh
   # or manually:
   mkdir build && cd build && cmake .. && cmake --build . --config Release
   ctest --output-on-failure
   ```

5. **Commit** your changes with a clear message, then **push** to your fork:

   ```bash
   git push origin feature/your-feature-name
   ```

6. **Open a Pull Request** against `main` on the upstream repository. Describe what you changed, why, and include screenshots for any visual changes.

### Areas Where Help Is Welcome

- Bug fixes and platform compatibility (macOS, Linux, Windows)
- New examples or live demos
- Physics improvements (collision, constraints, solvers)
- Rendering features (post-processing, materials, pipeline)
- Expanded test coverage (Audio, Input, Graphics)
- Documentation improvements and tutorials
- RL Backend enhancements (new algorithms, benchmarks)

### Code Style

- C++17, modern idioms (RAII, smart pointers)
- `PascalCase` for classes, `camelCase` for functions, `m_PascalCase` for members
- 4-space indentation, no tabs

### Reporting Issues

Use GitHub Issues for bugs and feature requests. For bugs, include your OS, compiler version, steps to reproduce, and any error output.

---

## License

MIT License. See [LICENSE](LICENSE) for details.
