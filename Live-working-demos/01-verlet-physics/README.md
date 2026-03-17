# 01 - Verlet Physics

Custom 2D/3D physics engine built from scratch using Verlet integration.
This was the core physics prototype for the LAGEngine project.

## Executables

| Target | Description |
| ------ | ----------- |
| `verlet_2d` | 2D cloth simulation with cannon projectiles (SFML) |
| `verlet_3d` | 3D cloth and ball physics with OpenGL rendering |

## Engine Modules

- **core** -- logger, math utilities, time management
- **physics** -- Verlet particles, constraint solver, springs, cloth solver, rigid body, collision detection (broadphase spatial hash, narrowphase SAT)
- **graphics** -- camera, shader, mesh, mesh generator, texture (OpenGL 3.3+)
- **objects** -- Ball2D/3D, Cloth2D/3D, Cannon2D
- **scene** -- scene graph, scene manager with object lifecycle

Also includes standalone demos in `src/`:

- `sph_water.cpp` -- single-file SPH water simulation (build with `g++ -O3 -std=c++17 sph_water.cpp -o sph_water -lglfw -lGLEW -lGL`)
- `testRigidBodies.cpp` -- rigid body physics unit test

## 2D Demo Controls

- Left/Right Arrow -- rotate cannon
- Up/Down Arrow -- adjust power
- Space -- fire ball
- R -- reset cloth

## 3D Demo Controls

- W/A/S/D -- move camera
- Mouse -- look around
- Space -- spawn ball
- R -- reset simulation

## Dependencies

- SFML 2.5 (2D executable)
- GLFW (3D executable)
- OpenGL 3.3+
- GLM

On Debian-based systems:

```bash
sudo apt install libsfml-dev libglfw3-dev libglm-dev libgl1-mesa-dev
```

## Build

```bash
mkdir build && cd build
cmake ..
make
```
