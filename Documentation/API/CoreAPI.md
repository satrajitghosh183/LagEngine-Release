# Core API Reference

## Application

The main entry point and lifecycle manager.

| Method | Description |
|--------|-------------|
| `Application::Get()` | Get the application singleton (throws if not initialized). |
| `Application::GetPtr()` | Get the application pointer (returns nullptr if not initialized). |
| `void Run()` | Main loop; runs until the window is closed. |
| `void Close()` | Request application shutdown. |
| `Window* GetWindowPtr()` | Get the main window (null if headless). |
| `SceneManager& GetSceneManager()` | Access the scene manager. |

## Logger

Thread-safe logging with levels: Trace, Debug, Info, Warning, Error, Critical.

| Macro | Level |
|-------|-------|
| `GE_TRACE(msg)` | Trace |
| `GE_DEBUG(msg)` | Debug |
| `GE_INFO(msg)` | Info |
| `GE_WARN(msg)` | Warning |
| `GE_ERROR(msg)` | Error |
| `GE_CRITICAL(msg)` | Critical |
| `GE_CORE_*` | Core engine logs |

## Input

- `Input::IsKeyPressed(KeyCode key)` – Key currently held.
- `Input::IsKeyDown(KeyCode key)` – Key just pressed this frame.
- `Input::IsKeyReleased(KeyCode key)` – Key just released.
- `Input::GetMousePosition()` – Mouse position (x, y).

## RuntimePaths

Asset/shader path resolution relative to the executable.

| Method | Description |
|--------|-------------|
| `GetExecutableDirectory()` | Directory of the running executable. |
| `GetShadersDirectory()` | Shaders root (install or build layout). |
| `GetContentDirectory()` | Content root. |
| `Resolve(path)` | Resolve a relative path to an absolute path. |
| `ResolveShader(path)` | Resolve a shader path (e.g. `basic.vert`). |
