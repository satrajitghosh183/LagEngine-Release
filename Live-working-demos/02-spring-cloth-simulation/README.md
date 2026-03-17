# 02 - Spring Cloth Simulation

Spring-mass cloth and flag simulation using the PartyKel physics library.
Interactive parameter tweaking via AntTweakBar.

## Features

- Hook force and brake force model for cloth dynamics
- Multiple demo executables built from `src/` (one per .cpp file)
- Real-time parameter adjustment (stiffness, damping, wind) via AntTweakBar GUI
- OpenGL rendering with SDL windowing

## Dependencies

System packages:

- SDL 1.2
- OpenGL
- GLEW

On Debian-based systems:

```bash
sudo apt install libsdl1.2-dev libglew-dev libgl1-mesa-dev
```

Vendored (included in source tree):

- AntTweakBar (in `third-party/`)
- GLM (in `third-party/include/`)

## Build

```bash
mkdir build && cd build
cmake ..
make
```

Each `.cpp` file in `src/` produces its own executable.

## Language

C++11
