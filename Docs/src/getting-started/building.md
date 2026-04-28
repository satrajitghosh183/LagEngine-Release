# Building LAG Engine

LAG Engine builds with CMake ≥ 3.20 and any C++17 compiler
(GCC 9+, Clang 10+, MSVC 19.25+).

## Dependencies

All vendored under `External/`:

- GLFW (windowing)
- GLM (math)
- Vulkan SDK (install separately — set `VULKAN_SDK` env var)
- VMA (Vulkan Memory Allocator)
- ImGui (editor UI)
- Lua 5.4
- nlohmann_json
- stb_image
- Assimp (optional)
- OpenAL-Soft (optional)

## Build

```bash
git clone https://github.com/satrajitghosh183/LAG-Engine.git
cd LAG-Engine
bash setup.sh
```

Or manually:

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## Options

| Option | Default | Description |
|--------|:-------:|-------------|
| `BUILD_EXAMPLES` | ON  | Build `Examples/` |
| `BUILD_EDITOR`   | ON  | Build the ImGui editor |
| `BUILD_TESTS`    | ON  | Build Google Test suite |
| `BUILD_SHARED_LIBS` | OFF | Build engine as shared libs |

## Run tests

```bash
cd build
ctest --output-on-failure
# or directly
./bin/GameEngineTests
```

## Platform notes

- **Windows**: needs Visual Studio 2019+ or Clang-cl. Link ws2_32 is handled automatically.
- **Linux**: needs X11/Wayland headers (`libx11-dev`, `libwayland-dev`) and pulseaudio for OpenAL.
- **macOS**: requires MoltenVK from the Vulkan SDK.
