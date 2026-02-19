# VerletX Engine Editor

A modern game engine editor built with ImGui and OpenGL, inspired by [LumixEngine](https://github.com/nem0/LumixEngine)'s editor architecture.

![Editor Preview](preview.png)

## Features

### Core Panels

- **Scene Hierarchy** - View and manage all objects in your scene with tree structure
- **Inspector** - Edit properties of selected objects (Transform, RigidBody, Cloth, etc.)
- **Viewport** - 3D scene rendering with camera controls and gizmos
- **Console** - Log messages, warnings, and errors with filtering
- **Asset Browser** - Browse, search, and organize project assets
- **Physics Settings** - Configure physics simulation parameters
- **Performance Monitor** - FPS, memory usage, and detailed timing breakdown

### Key Features

- **Docking System** - Fully customizable panel layout using ImGui docking
- **Multiple Themes** - Dark, Nord, Dracula, CyberPunk, Forest, and more
- **Real-time Physics** - Play/Pause/Step simulation controls
- **Gizmo Tools** - Translate, Rotate, Scale with local/world space toggle
- **Search & Filter** - Quick search in hierarchy and console
- **Drag & Drop** - Asset drag-drop support between panels

## Building

### Prerequisites

- CMake 3.16+
- C++17 compatible compiler
- OpenGL 4.3+

### Build Instructions

```bash
# From the Editor directory
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Running

```bash
./bin/VerletXEditor
```

## Controls

### Viewport
- **Right Click + WASD** - Camera movement
- **Right Click + Mouse** - Camera look around
- **Scroll** - Zoom in/out
- **F** - Focus on selected object
- **W/E/R** - Switch between Translate/Rotate/Scale gizmos

### Shortcuts
- **Ctrl+N** - New scene
- **Ctrl+O** - Open scene
- **Ctrl+S** - Save
- **Ctrl+Z** - Undo
- **Ctrl+Y** - Redo
- **F5** - Play/Stop simulation
- **F6** - Step simulation

## Architecture

```
Editor/
├── core/
│   ├── EditorApp.hpp/cpp       # Main application class
│   └── EditorTheme.hpp/cpp     # Theming and styling
├── panels/
│   ├── EditorPanel.hpp         # Base panel class
│   ├── SceneHierarchyPanel.*   # Scene tree view
│   ├── InspectorPanel.*        # Property editor
│   ├── ViewportPanel.*         # 3D viewport
│   ├── ConsolePanel.*          # Log console
│   ├── AssetBrowserPanel.*     # Asset management
│   ├── PhysicsPanel.*          # Physics settings
│   └── PerformancePanel.*      # Performance monitoring
├── widgets/                     # Reusable UI widgets
├── utils/                       # Utility functions
├── resources/
│   ├── icons/                   # UI icons
│   └── fonts/                   # Custom fonts
├── main.cpp                     # Entry point
└── CMakeLists.txt               # Build configuration
```

## Integration with Engine

The editor is designed to work with the VerletX physics engine components:

- `engine::physics::RigidBody` - Rigid body physics
- `engine::physics::PhysicsWorld` - Physics simulation world
- `engine::objects::Cloth3D` - Cloth simulation
- `engine::graphics::Camera` - 3D camera
- `engine::scene::Scene3D` - Scene management

## Themes

Available themes:
- **Dark** - Default dark theme
- **Dark Pro** - Professional dark with cyan accents
- **Light** - Clean light theme
- **Nord** - Nord color palette
- **Dracula** - Dracula color scheme
- **CyberPunk** - Neon cyberpunk aesthetic
- **Forest** - Nature-inspired green theme

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI
- [GLFW](https://github.com/glfw/glfw) - Window and input handling
- [GLAD](https://glad.dav1d.de/) - OpenGL loader
- [GLM](https://github.com/g-truc/glm) - Mathematics library

## Credits

Inspired by:
- [LumixEngine](https://github.com/nem0/LumixEngine) - Open source 3D game engine
- Unity Editor
- Unreal Engine Editor

## License

This project is part of the MasterThesis_FinalProject and follows the same license as the parent project.

