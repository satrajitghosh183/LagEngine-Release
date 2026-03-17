# 07 - Robot Arm Kinematics

Robot arm simulation with forward kinematics (FK) and inverse kinematics (IK)
using Denavit-Hartenberg parameters and PID control.

## Features

- Denavit-Hartenberg parameter model for joint chain representation
- Forward kinematics with DH matrix multiplication
- Inverse kinematics solver (Jacobian-based)
- PID controller for smooth joint tracking
- Interactive ImGui panel for parameter tweaking
- ImGuizmo 3D manipulation gizmos
- Mesh rendering for joint links, spheres, cylinders, and boxes

## Dependencies

All dependencies are bundled in `lib/`:

- Eigen (linear algebra)
- GLEW (OpenGL extension loading)
- GLFW (windowing)
- ImGui (debug UI)
- ImGuizmo (3D gizmos)

On Windows, prebuilt GLEW and GLFW libraries are included. On Linux, install
system packages:

```bash
sudo apt install libglew-dev libglfw3-dev libgl1-mesa-dev
```

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Controls

- ImGui panels control joint angles, PID gains, and IK target position
- ImGuizmo gizmo for manipulating the IK target in 3D space

## Language

C++17
