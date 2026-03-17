# 03 - XPBD Soft Body

Extended Position-Based Dynamics (XPBD) soft body physics demo with PBR rendering.
Implements an HXPBD constraint solver for real-time deformable body simulation.

## Features

- HXPBD constraint solver with substep iteration
- Dihedral bend constraints for surface rigidity
- Global volume preservation constraints
- PBR materials with shadow mapping
- Directional lighting
- Orbit camera controls
- ImGui debug interface

## Controls

- Mouse drag: rotate the camera
- Ctrl + mouse drag: pan the camera
- Mouse wheel: zoom in/out
- Shift + mouse drag: lock rotation to pick an object

## Dependencies

System packages:

- GLFW
- GLM

On Debian-based systems:

```bash
sudo apt install libglfw3-dev libglm-dev
```

Vendored (included in source tree):

- GLAD (OpenGL loader)
- Eigen (linear algebra)
- ImGui (debug UI)

## Build

```bash
mkdir build && cd build
cmake ..
make
```
