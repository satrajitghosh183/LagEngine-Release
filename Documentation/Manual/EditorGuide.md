# GameEngine Editor User Guide

## Overview

The GameEngine Editor provides a comprehensive Wicked Engine-style editing environment for creating and managing game scenes, assets, and scripts.

## Key Features

### Scene Management

- **Multiple Scene Tabs**: Work with multiple scenes simultaneously using the tab system at the top of the editor
  - Click a tab to switch between scenes
  - Click the "x" button to close a scene (will prompt to save if there are unsaved changes)
  - Click the "+" button to create a new scene
  - Unsaved changes are indicated by an asterisk (*) next to the scene name
  - Tooltips show the full file path when hovering over tabs

### Scripting Console

- **Toggle**: Press the **HOME** key to toggle the scripting console visibility
- **Features**:
  - Execute Lua scripts directly in the editor
  - View command history and output
  - Multi-line script input support
  - Error display and logging

### Asset Browser

- **Location**: Bottom panel (docked by default)
- **Features**:
  - Directory tree navigation
  - Grid and list view modes
  - Filtering by asset type (All, Models, Scripts, Textures, Audio, Scenes, Shaders)
  - Sorting options (Recent, Name, Path, Size, Type)
  - Recent folders dropdown for quick navigation
  - Search with case-sensitive option
  - Drag-and-drop assets onto the viewport to import them

### Render Paths

- **Location**: Top-right of viewport panel
- **Features**:
  - Switch between RenderPath3D and RenderPath2D
  - Post-process toggles for RenderPath3D:
    - **FXAA**: Fast Approximate Anti-Aliasing
    - **Bloom**: Bloom post-processing effect
    - **SSAO**: Screen-Space Ambient Occlusion

### Layout Management

- **Reset Layout**: Use `Window > Reset Layout` to restore the default docking layout
- **Customization**: All panels can be resized, reordered, and undocked
- **Floating Windows**: Panels can be torn off into separate windows

### Theme Editor

- **Location**: `Window > Theme Editor`
- **Features**:
  - Customize ImGui colors, sizing, rounding, and alpha values
  - Apply preset themes (Default/Dark, Light, Classic)
  - Save and load custom themes

## Command-Line Options

The editor supports various command-line graphics options:

- `OPENGL` - Use OpenGL renderer
- `VULKAN` - Use Vulkan renderer (if available)
- `debugdevice` - Enable debug device logging
- `gpuvalidation` - Enable GPU validation layers
- `gpu_verbose` - Enable verbose GPU logging
- `igpu` - Prefer integrated GPU
- `amdgpu` - Prefer AMD GPU
- `nvidiagpu` - Prefer NVIDIA GPU
- `intelgpu` - Prefer Intel GPU
- `alwaysactive` - Keep running when window is not focused

## Hotkeys

- **HOME**: Toggle scripting console
- **Ctrl+Z**: Undo
- **Ctrl+Y**: Redo
- **Ctrl+N**: New scene
- **Ctrl+O**: Open scene
- **Ctrl+S**: Save scene
- **Ctrl+Shift+S**: Save scene as
- **W**: Translate gizmo
- **E**: Rotate gizmo
- **R**: Scale gizmo
- **Q**: No gizmo
- **F**: Focus camera on selected entity

## Model Import

Supported formats:
- OBJ
- FBX
- GLTF/GLB
- DAE (Collada)
- VRM/VRMA (if supported)

Models can be imported by:
1. Dragging from Asset Browser to Viewport
2. Using `scene.LoadModel()` in Lua scripts
3. Using `wi::scene::LoadModel()` in C++

## Scripting

See `ScriptingAPI.md` for detailed scripting documentation.

The editor automatically executes `startup.lua` if present in:
- Working directory
- `Scripts/` folder
- `Assets/Scripts/` folder
