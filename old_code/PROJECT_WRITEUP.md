# Master Thesis Final Project — Complete Technical Writeup

**Author:** Satrajit Ghosh  
**Course:** Computer Graphics, Spring Semester 2025  
**Project:** MasterThesis_FinalProject

---

## Executive Summary

This repository is a comprehensive Computer Graphics research portfolio that spans **physics simulation**, **game engine architecture**, **GPU computing (CUDA)**, **reinforcement learning**, **robotics kinematics**, **fluid dynamics**, and **AI-assisted pipeline optimization**. The project includes a custom game engine (LAG Engine), multiple simulation systems, and integrations with industry-standard tools and third-party engines.

---

## Table of Contents

1. [Project Structure Overview](#1-project-structure-overview)
2. [LAG Engine (Custom Game Engine)](#2-lag-engine-custom-game-engine)
3. [CUDA_GL_Demo — GPU Particle Simulation + RL](#3-cuda_gl_demo--gpu-particle-simulation--rl)
4. [arm-simulation — Robotic Arm Kinematics](#4-arm-simulation--robotic-arm-kinematics)
5. [cloth-simulation — PartyKel Flag Simulator](#5-cloth-simulation--partykel-flag-simulator)
6. [Demo2 — Deferred Rendering & AI Pipeline](#6-demo2--deferred-rendering--ai-pipeline)
7. [Editor — VerletX Engine Editor](#7-editor--verletx-engine-editor)
8. [feather — XPBD Soft Body Physics](#8-feather--xpbd-soft-body-physics)
9. [particle-sim — Basic Particle Physics](#9-particle-sim--basic-particle-physics)
10. [SPH Water Simulation](#10-sph-water-simulation)
11. [Third-Party / Reference Projects](#11-third-party--reference-projects)
12. [Backup — Legacy 2D/3D Code](#12-backup--legacy-2d3d-code)
13. [Documents & Deliverables](#13-documents--deliverables)
14. [Future Roadmap (from plan.md)](#14-future-roadmap-from-planmd)

---

## 1. Project Structure Overview

```
MasterThesis_FinalProject/
├── engine/              # LAG Engine core (physics, graphics, scene)
├── src/                 # Main executables: main2D, main3D, sph_water
├── shaders/             # GLSL shaders for cloth, ball, general
├── CUDA_GL_Demo/        # CUDA + OpenGL particles + RL optimization
├── arm-simulation/      # Robot arm FK/IK with DH parameters
├── cloth-simulation/    # PartyKel cloth/flag simulation
├── Demo2/               # Deferred shading, render graph, AI backend
├── Editor/              # VerletX Engine Editor (ImGui-based)
├── feather/             # XPBD/HXPBD soft body engine
├── particle-sim/        # Simple freeglut particle demo
├── bulletOpenGL/        # Bullet Physics + OpenGL demos (reference)
├── stella-engine/       # 2D game engine with Lua scripting (reference)
├── Backup/              # Legacy Verlet 2D/3D implementations
├── Documents/           # Reports, presentations, PDFs
├── external/            # GLAD, stb_image, etc.
├── build2/              # Build artifacts (main2D, main3D)
├── CMakeLists.txt       # Root build configuration
├── plan.md              # Thesis roadmap (Agentic AI, Generative AI, Symbiotic AI)
└── README.md            # LAG Engine documentation
```

---

## 2. LAG Engine (Custom Game Engine)

**Location:** `engine/`, `src/main2D.cpp`, `src/main3D.cpp`, `shaders/`

### 2.1 Overview

LAG Engine is a **custom-built game development framework** designed from first principles, supporting both **2D and 3D physics simulations** with a unified architecture.

### 2.2 Core Architecture

| Module | Path | Purpose |
|--------|------|---------|
| **Core** | `engine/core/` | Logger, MathUtils, RandomUtils, Time |
| **Graphics** | `engine/graphics/` | Camera, Mesh, Mesh3D, MeshGenerator, Shader, Texture2D |
| **Physics** | `engine/physics/` | Verlet integration, cloth solver, rigid bodies, constraints |
| **Objects** | `engine/objects/` | Ball2D, Ball3D, Cloth2D, Cloth3D, Cannon2D |
| **Scene** | `engine/scene/` | Scene, Scene2D, Scene3D, SceneManager2D/3D |
| **Input** | `engine/input/` | InputManager |

### 2.3 Physics Systems

#### Verlet Integration
- **VertletSystem** (2D): SFML-based 2D particle system with constraints, gravity, damping
- **VertletSystem3D**: 3D extension with `Particle3D`, `Constraint3D`, collision handling
- **ClothSolver** / **ClothSolver3D**: Constraint-based cloth dynamics with configurable iterations, tearing support via stress thresholds

#### Rigid Body Physics
- **RigidBody**, **PhysicsWorld**, **PhysicsWorld3D**
- **Collision shapes**: BoxShape, SphereShape, CapsuleShape
- **Collision detection**: BroadPhase, SpatialHashBroadPhase for efficient potential collision pair finding
- **ContactManifold** for contact resolution

#### Constraint Systems
- **Constraint2D** / **Constraint3D**: Distance constraints
- **Spring3D**: Spring-damper connections
- **Joints**: DistanceJoint, HingeJoint (in `physics/constraints/`)

#### Soft Body
- **SoftBodySystem** / **SoftBodySystem3D**: Deformable particle-constraint systems

#### Character Controller
- **CharacterController** for platformer-style movement

### 2.4 Rendering

- **Phong lighting** (ambient, diffuse, specular)
- **Dynamic mesh generation** for cloth and deformable objects
- **Texture mapping** via Texture2D and stb_image
- **2D rendering**: SFML for main2D
- **3D rendering**: OpenGL 3.3+ (GLAD, GLFW) for main3D

### 2.5 Demos

- **main2D**: 2D cloth with cannon projectiles (arrow keys to rotate cannon, Space to fire, R to respawn cloth)
- **main3D**: 3D cloth + ball physics, WASD camera, Space to spawn balls, R to reset

### 2.6 Dependencies

- C++17, CMake 3.16+
- OpenGL 3.3+, GLAD, GLFW
- SFML 2.5 (graphics, window, system)
- GLM (math)
- stb_image (textures)

---

## 3. CUDA_GL_Demo — GPU Particle Simulation + RL

**Location:** `CUDA_GL_Demo/`

### 3.1 Overview

A **CUDA-accelerated particle simulation** rendered with OpenGL, with an optional **Reinforcement Learning interface** for optimizing GPU resource allocation (block size, grid configuration).

### 3.2 CUDA Simulation (`src/cuda/`)

| File | Purpose |
|------|---------|
| **sim.cuh** | `SimParams` struct, kernel launch declarations |
| **sim.cu** | `kInit`, `kStep` CUDA kernels |

**SimParams:**
- `dt`, `gravityY`, `restitution`, `damping`
- `emitY`, `floorY` — emission region and floor
- `seed`, `frame` — for deterministic random re-emission

**Physics:**
- Gravity applied in Y
- Floor collision with bounce (restitution, damping)
- Color changes: velocity-based (blue→red gradient), yellow on impact
- **Re-emission**: When particles settle (y near floor, low velocity), they are re-emitted from a fountain-like region with randomized spherical velocity

**Kernels:**
- `kInit`: Spreads particles with random spherical emission velocities
- `kStep`: Integrates positions, applies floor collision, updates colors, handles re-emission

### 3.3 OpenGL Rendering

- **Renderer** (`renderer.cpp/hpp`): VBOs for positions (float4) and colors (float4), point-based rendering
- **gl_utils**: OpenGL setup helpers
- **main.cpp**: GLFW window, orthographic view, performance statistics (CUDA time, CPU copy, GL upload, render time)

### 3.4 Performance Pipeline

1. **CUDA Step**: `launchStep()` — runs simulation on GPU
2. **CPU Copy**: `cudaMemcpy` positions and colors to host
3. **GL Upload**: `glBufferSubData` to VBOs
4. **Rendering**: Draw points with view-projection matrix

Statistics printed: FPS, frame time, CUDA/CPU/upload/render breakdown.

### 3.5 RL Interface (`rl_interface/`)

| File | Purpose |
|------|---------|
| **CudaGLEnvironment.py** | Gymnasium environment for CUDA GL Demo |
| **train_rl.py** | PPO training script |
| **evaluate_rl.py** | Evaluate trained models |
| **run_optimal.py** | Run demo with optimal learned settings |
| **collect_data.py** | Collect metrics across particle counts and block sizes |
| **example_usage.py** | Example scripts |
| **test_executable.py** | Test cuda_gl_demo_rl executable |
| **requirements.txt** | stable-baselines3, gymnasium, numpy, etc. |

**Environment Design:**
- **Action space**: `[block_size_idx (0–3), grid_size_factor (0–1)]` → block sizes [128, 256, 512, 1024]
- **Observation space**: FPS, frame_time, cuda_time, cpu_time, upload_time, render_time, block_size_norm, gpu_utilization_estimate
- **Reward**: FPS reward + GPU utilization reward + efficiency reward − frame_time penalty − FPS penalty

**Goal:** Optimize CUDA block size for maximum GPU utilization and FPS under high particle count stress (e.g., 500,000 particles).

---

## 4. arm-simulation — Robotic Arm Kinematics

**Location:** `arm-simulation/`

### 4.1 Overview

A **robotic arm simulation** with **forward kinematics (FK)** and **inverse kinematics (IK)**, driven by Denavit-Hartenberg (DH) parameters, with PID joint control and OpenGL rendering.

### 4.2 Core Components

| File | Purpose |
|------|---------|
| **Robot.hpp/cpp** | Robot model, joint angles, TCP position, Jacobian, IK target |
| **JointedLink.hpp/cpp** | Single link with DH params, joint rotation |
| **DhParam.hpp/cpp** | Denavit-Hartenberg parameters (joint type, a, d, α, θ) |
| **InverseKinematics.hpp/cpp** | IK solver (SimpleIKSolver with distance threshold, timeout) |
| **PidController.hpp/cpp** | PID control for joint angles |
| **Application.hpp/cpp** | Main loop, rendering, GUI sliders, IK target setting |

### 4.3 Kinematics

- **FK**: `getTcpWorldPosition()`, `getJacobian()` — world frame TCP position and Jacobian
- **IK**: `solveInverseKinematics(tcp_pos)` — returns joint angles; `IKSolution` includes `joint_angles`, `time_taken_micros`, `timed_out`

### 4.4 DH Parameters (`resources/params.txt`)

6-DOF arm with revolute joints:
- Joint types: R (revolute)
- Parameters: s (joint type), a, d, alpha, theta (variable x for joints)

### 4.5 Rendering

- **Camera**, **Shader** (simple_shader.vert/frag)
- **Meshes**: BoxMesh, CylinderMesh, SphereMesh, JointedLinkMesh
- Draw modes: MESH, MESH_WIREFRAME

### 4.6 Controls

- Mouse: set IK target
- Joint sliders for manual control
- PID gains: `pid_p`, `pid_i`, `pid_d` (tunable in Application)

---

## 5. cloth-simulation — PartyKel Flag Simulator

**Location:** `cloth-simulation/`

### 5.1 Overview

A **3D cloth/flag simulation** built on the PartyKel framework, with octree spatial structure and 3D rendering.

### 5.2 Structure

| Component | Path | Purpose |
|-----------|------|---------|
| **PartyKel** | `PartyKel/` | Core library |
| **FlagRenderer3D** | `PartyKel/renderer/FlagRenderer3D.hpp` | Renders cloth grid from position array |
| **Renderer3D** | `PartyKel/renderer/Renderer3D.cpp` | 3D scene rendering |
| **Octree** | `PartyKel/Octree.hpp` | Spatial partitioning (depth, position, dimension, children) |
| **WindowManager** | `PartyKel/WindowManager.cpp` | Window and input |
| **AntTweakBar** | `third-party/AntTweakBar/` | Parameter tweaking UI |

### 5.3 Octree

- Template class for spatial indexing
- Depth, position, dimension
- Children and leaf values
- Used for collision or neighbor queries

### 5.4 FlagRenderer3D

- `drawGrid(const glm::vec3* positionArray, bool wireframe)` — renders cloth from particle positions
- MVP matrices for projection and view

### 5.5 Dependencies

- GLEW, GLM
- AntTweakBar for GUI

---

## 6. Demo2 — Deferred Rendering & AI Pipeline

**Location:** `Demo2/`

### 6.1 Overview

An **advanced rendering demo** with **deferred shading**, **render graph**, **DAG-based scheduling**, and an **AI backend** for predictive pipeline optimization.

### 6.2 Rendering Pipeline

| Component | Purpose |
|-----------|---------|
| **GBuffer** | Geometry buffer (positions, normals, etc.) |
| **ShadowMap** | Shadow mapping |
| **SSAO** | Screen-space ambient occlusion |
| **Framebuffer** | Off-screen rendering |
| **PostFX** | Post-processing (postfx.vert/frag) |
| **Lighting** | Deferred lighting pass (lighting.vert/frag) |

### 6.3 Render Graph

- **RenderGraph**: DAG of render tasks
- **RenderTask**: TaskType, cost estimate, TaskExecutor (OpenGL_MainThread, etc.)
- **DAGScheduler**: Topological sort, level-based execution
- Dependencies between tasks (e.g., GBuffer → SSAO → Lighting)

### 6.4 AI Backend (`AIBackend.h/cpp`)

**Purpose:** Predictive pipeline bubble management without ML training.

**Features:**
- **FrameMetrics**: cpuTime, gpuTime, frameTime, gpuUtilization, streamCount
- **Prediction**: `predictedBottleneck`, `recommendedStreamCount`, `shouldInjectCompute`, `confidence`, `reasoning`
- **BottleneckType**: CPUBound, GPUUnderutilized, Balanced

**Methods:**
- Exponential moving averages (short/long) for CPU/GPU
- Trend detection, volatility, CPU–GPU correlation
- Anomaly detection
- Optimal stream count recommendation
- Pattern recognition (trend, volatility, correlation, isAnomaly)

### 6.5 Other Components

- **Camera**, **Mesh**, **Shader**
- **ParticleSystem**: Particle rendering
- **PipelineController**: Orchestrates pipeline execution
- **Profiler**: Timing
- **UI**: ImGui interface
- **DemoScene**: Scene setup

### 6.6 Shaders

- `gbuffer.vert/frag`, `lighting.vert/frag`, `particle.vert/frag`
- `postfx.vert/frag`, `shadow.vert/frag`

---

## 7. Editor — VerletX Engine Editor

**Location:** `Editor/`

### 7.1 Overview

A **modern game engine editor** built with **ImGui** and **OpenGL**, inspired by LumixEngine.

### 7.2 Panels

| Panel | Purpose |
|-------|---------|
| **SceneHierarchyPanel** | Tree view of scene objects |
| **InspectorPanel** | Edit Transform, RigidBody, Cloth, etc. |
| **ViewportPanel** | 3D viewport with camera controls and gizmos |
| **ConsolePanel** | Logs with filtering |
| **AssetBrowserPanel** | Browse and organize assets |
| **PhysicsPanel** | Physics simulation parameters |
| **PerformancePanel** | FPS, memory, timing breakdown |

### 7.3 Features

- Docking system (ImGui docking)
- Multiple themes: Dark, Nord, Dracula, CyberPunk, Forest
- Play/Pause/Step for physics
- Gizmos: Translate, Rotate, Scale (W/E/R)
- Search and filter in hierarchy and console
- Drag-and-drop assets

### 7.4 Controls

- **Viewport**: Right-click + WASD (move), mouse (look), scroll (zoom), F (focus)
- **Shortcuts**: Ctrl+N/O/S, Ctrl+Z/Y, F5 (Play/Stop), F6 (Step)

### 7.5 Integration

Designed for VerletX / LAG Engine: RigidBody, PhysicsWorld, Cloth3D, Camera, Scene3D.

---

## 8. feather — XPBD Soft Body Physics

**Location:** `feather/`

### 8.1 Overview

A small **OpenGL engine** with **Position-Based Dynamics (PBD)** and **XPBD** (extended PBD) for soft body simulation.

### 8.2 Rendering

- Blinn-Phong and PBR shading
- Normal mapping, shadow mapping
- Post-processing, compute shaders
- Raw OBJ loading
- Up to 128 point lights
- ImGui integration
- CPU mouse picking

### 8.3 Physics (`core/physics/`)

- **HXPbdSolver**: XPBD with HPBD multigrid
- **SoftBody**: XPBD soft body
- **Constraints**: Distance, FastBend, DihedralBend, Volume, GlobalVolume, Collision, SelfCollision, Fixed, Area
- **CollisionConstraint** with friction
- **Collision LoD** using HPBD
- **UniformAccelerationField** (gravity)
- **RigidBody** support

### 8.4 Dependencies

- GLFW3, GLM
- Eigen (math)

---

## 9. particle-sim — Basic Particle Physics

**Location:** `particle-sim/`

### 9.1 Overview

A **simple particle simulation** using **freeglut** and OpenGL with basic gravity and friction.

### 9.2 Structure

- **Source.cpp**: Main loop, GLUT callbacks, particle list
- **Particle.h**: Particle state
- **Floor.h**: Floor collision
- **Line.h**: Line primitives

### 9.3 Features

- Cannon-style particle emission
- Gravity, friction
- Floor collisions
- Configurable: particle count, colors, paths, bumping
- Light source, shading modes
- Pause/resume

### 9.4 Dependencies

- freeglut
- C++03+
- X11 or compatible window manager

---

## 10. SPH Water Simulation

**Location:** `src/sph_water.cpp`

### 10.1 Overview

A **single-file SPH (Smoothed Particle Hydrodynamics) water simulation** with OpenGL billboard rendering.

### 10.2 SPH Model

- ~18,000 particles (configurable)
- Smoothing length `h`, rest density, pressure coefficient `k`, viscosity
- Uniform grid for neighbor search
- Box domain with boundary handling
- Bounce coefficient

### 10.3 Rendering

- Camera with target, distance, yaw, pitch
- Billboard spheres for particles
- Mouse: LMB rotate, RMB pan, scroll zoom

### 10.4 Build

```bash
g++ -O3 -std=c++17 sph_water.cpp -o sph_water -lglfw -lGLEW -lGL -ldl -lpthread -lX11 -lXrandr -lXi
```

---

## 11. Third-Party / Reference Projects

### 11.1 bulletOpenGL

**Location:** `bulletOpenGL/`

- Learning project: Bullet Physics + OpenGL
- Demos: Basic physics, collision filtering, raycasting, constraints, trigger volumes, soft body
- Uses FreeGLUT, Bullet 3.x
- Covers rigid bodies, convex hulls, compound shapes, collision events

### 11.2 stella-engine

**Location:** `stella-engine/`

- Cross-platform 2D C++/OpenGL game engine
- Features: Batch rendering, spritesheets, animations, particles, framebuffers, bitmap/TTF fonts, audio streaming, platformer physics, Lua scripting, ImGui, map editor, NLP module
- Includes **NLP sandbox** (lemmatizer, tokenizer) and **normal map sandbox**
- Nikte game: Dialogue system, NPCs, Lua scripts, tile-based maps

---

## 12. Backup — Legacy 2D/3D Code

**Location:** `Backup/`

Legacy implementations of:
- **2D**: Ball, Cloth, Particle, VerletSystem, MeshGenerator
- **3D**: Ball3D, Cloth3D, Particle3D, Camera3D, Shader
- Headers: Object3D, number_generator, math

Predecessor to the current engine architecture.

---

## 13. Documents & Deliverables

**Location:** `Documents/`

- **Computer_Graphics Sarajit_Ghosh_Final Report.pdf** — Final report
- **FinalPresentationComputerGraphics.pdf** — Presentation
- **FinalPresentationComputerGraphics.pptx** — Presentation source

**Root:**
- **MasterThesisObjectivesLIst_SatrajitGhosh.pdf** — Thesis objectives
- **Plan_diagram_temp.png** — Planning diagram

---

## 14. Future Roadmap (from plan.md)

### Phase 2: Agentic AI — Autonomous Playtesters

- Agent API for querying physics, scene, applying actions
- Perception (radius queries, line-of-sight, volumetric overlap)
- Stress-test, playtest, and validation agents

### Phase 3: Generative AI — Automated Content Creation

- Procedural texture generation (e.g., diffusion models)
- AI-assisted model generation
- Scene reconstruction (NeRF / Gaussian Splatting)

### Phase 4: Symbiotic AI — Smart Editor & Designer

- Natural language editor commands
- AI debugging assistant
- Agent-guided world building
- Intelligent shader generation from descriptions

---

## Summary Table

| Component | Primary Tech | Key Idea |
|-----------|--------------|----------|
| LAG Engine | C++17, OpenGL, SFML, Verlet | Custom game engine with 2D/3D physics |
| CUDA_GL_Demo | CUDA, OpenGL, Gymnasium | GPU particles + RL for block size optimization |
| arm-simulation | Eigen, OpenGL, DH params | Robot arm FK/IK with PID control |
| cloth-simulation | GLEW, GLM, Octree | 3D flag/cloth with PartyKel |
| Demo2 | OpenGL, DAG | Deferred rendering + AI pipeline optimization |
| Editor | ImGui, OpenGL | VerletX/LAG Engine editor |
| feather | XPBD, OpenGL | Soft body simulation |
| particle-sim | freeglut | Simple particle demo |
| sph_water | SPH, OpenGL | CPU fluid simulation |
| bulletOpenGL | Bullet, OpenGL | Physics learning demos |
| stella-engine | 2D, Lua, NLP | 2D engine reference |

---

*This document was generated from a full codebase analysis. For build instructions, see individual README files and the root CMakeLists.txt.*
