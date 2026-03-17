# 08 - ImGui Editor

An ImGui-based scene editor prototype originally built for a Verlet physics
engine. This demo served as the foundational prototype for the LAGEngine editor
interface.

## Features

### Panels

- **Scene Hierarchy** -- tree view of all entities in the scene with
  add/remove/reparent controls and quick search.
- **Inspector** -- property editor for the selected entity's components
  (Transform, RigidBody, Cloth, etc.).
- **Viewport** -- 3D scene rendered into an offscreen framebuffer and displayed
  inside an ImGui image widget, with mouse-based camera controls and gizmos.
- **Console** -- filterable log output for messages, warnings, and errors.
- **Asset Browser** -- file-system view for browsing, searching, and organizing
  project assets with drag-and-drop support.
- **Physics Panel** -- controls for gravity, iteration count, constraint
  stiffness, and other solver parameters; play/pause/step simulation.
- **Performance Panel** -- FPS counter, memory usage, and per-system timing
  breakdown.

### Editor features

- Docking system with fully customizable panel layout.
- Multiple themes (Dark, Nord, Dracula, CyberPunk, Forest, and others).
- Translate / Rotate / Scale gizmos with local and world space toggle.
- Keyboard shortcuts for common operations (Ctrl+S, Ctrl+Z, F5 play/stop, etc.).

## Dependencies

| Dependency | Source |
|---|---|
| GLFW | Fetched automatically via CMake FetchContent |
| GLAD | Vendored in `external/` |
| Dear ImGui | Vendored in `external/` |

## Build

Requires a C++17 compiler and CMake 3.16+.

```bash
mkdir build && cd build
cmake ..
make
```

GLFW will be downloaded and built automatically during the CMake configure step.

## Controls

### Viewport

- **Right Click + WASD** -- camera movement
- **Right Click + Mouse** -- camera look
- **Scroll** -- zoom
- **F** -- focus on selected object
- **W / E / R** -- translate / rotate / scale gizmo

### Shortcuts

- **Ctrl+N** -- new scene
- **Ctrl+O** -- open scene
- **Ctrl+S** -- save
- **Ctrl+Z / Ctrl+Y** -- undo / redo
- **F5** -- play / stop simulation
- **F6** -- step simulation

## Language

C++17
