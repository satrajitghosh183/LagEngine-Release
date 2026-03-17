# Getting Started with GameEngine

This guide walks you from zero to a running scene in under 30 minutes.

## Download and Install

### Option 1: Pre-built release (recommended)

1. Go to the [Releases](https://github.com/satrajitghosh183/LAGEngine/releases) page.
2. Download the installer or ZIP for your platform:
   - **Windows:** `GameEngine-x.x.x-Windows.exe` (NSIS installer) or `.zip` for portable.
   - **macOS:** `GameEngine-x.x.x-macOS.dmg` or `.zip`.
   - **Linux:** `gameengine_x.x.x_amd64.deb`, `.rpm`, or `.tar.gz`.
3. Run the installer or extract the ZIP. The Editor executable is in the `bin/` folder (e.g. `bin/GameEngineEditor` or `GameEngineEditor.exe`).
4. Double-click the Editor to launch.

### Option 2: Build from source

See the root [README.md](../../README.md) for prerequisites (CMake, compiler, OpenAL, etc.).

```bash
# Clone (with submodules)
git clone --recursive https://github.com/satrajitghosh183/LAGEngine.git
cd GameEngine

# Configure and build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EDITOR=ON -DBUILD_EXAMPLES=ON
cmake --build . -j
```

Run the Editor from `build/bin/GameEngineEditor` (or `GameEngineEditor.exe` on Windows).

## First Launch

When you open the Editor you should see:

- **Left:** Scene Hierarchy (list of entities in the current scene).
- **Center:** Viewport (3D view) and optionally Asset Browser / Console below.
- **Right:** Components (properties of the selected entity).
- **Top:** Menu bar (File, Edit, Entity, View, Window, Help) and a toolbar with Play / Pause / Stop.

If you have no scene open, you start with one empty “Untitled Scene” tab.

## Your First Scene

1. **Add an entity:** In the Scene Hierarchy, click the “+” or use **Entity → Create Empty**. Name it e.g. “Cube”.
2. **Add a mesh:** With the entity selected, in the Components panel click **+ Add Component → Mesh Renderer**. In the Mesh section click **Select Mesh…** and pick a built-in mesh (e.g. from your Assets) or the engine will use a default if available.
3. **Add a camera:** **Entity → Camera** (or Create Empty and add a Camera component). In the Camera component, check **Main Camera** so the viewport uses it.
4. **Position the camera:** Select the camera entity, then in the Transform component set Position to something like `(0, 2, 5)` so you can see the cube.
5. **Press Play** in the toolbar. The scene runs in play mode; press Stop to return to editing.

You can save the scene with **File → Save Scene** (or Save As) to a `.scene` file.

## Next Steps

- **Editor features:** See [EditorGuide.md](EditorGuide.md) for panels, hotkeys, and workflows.
- **Scripting:** See [ScriptingAPI.md](ScriptingAPI.md) for Lua scripting and the scripting console.
- **Tutorials:** See [../Tutorials/](../Tutorials/) for step-by-step tutorials (First Scene, Physics, Lua, Shaders, Cloth).

## Troubleshooting

- **Black viewport:** Ensure a camera exists and is set as Main Camera, and that it looks at your content (check Transform and FOV).
- **Missing shaders:** If you built from source, run the Editor from the `build` directory or copy `Assets/Shaders` next to the executable so the engine can find them.
- **Crashes on startup:** Update GPU drivers; on Linux install `libgl1-mesa-dev` and related packages (see README).
