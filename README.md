# 🎮 GameEngine

Professional 3D Game Engine built in C++17.

## Features

- 🎨 Modern 3D Renderer (OpenGL)
- ⚡ Physics Engine (Rigid body, Soft body, Fluids)
- 🎬 Animation System
- 🎵 3D Audio (OpenAL)
- ✨ Particle System
- 🎮 Input System
- 📝 Lua Scripting
- 🖥️ ImGui UI

---

## 🍎 macOS Setup (Quick Start)

### Prerequisites

Install dependencies via Homebrew:

```bash
# Install Homebrew if you don't have it
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install required tools and libraries
brew install cmake
brew install llvm           # For modern C++17 compiler
brew install openal-soft    # Audio library
brew install pkg-config     # For finding libraries
```

### Clone and Build

```bash
# Clone the repository
git clone git@github.com:satrajitghosh183/Theisis_test.git
cd Theisis_test

# Create build directory and build
mkdir build && cd build
cmake ..
cmake --build . -j$(sysctl -n hw.ncpu)
```

### Run Examples

```bash
# From the build directory
./bin/HelloTriangle
./bin/PhysicsDemo
./bin/ClothSimulation
./bin/FluidSimulation
./bin/CharacterController
./bin/RobotArm
./bin/GameEngineEditor
```

---

## 🪟 Windows Setup

### Prerequisites

- **Visual Studio 2019+** with C++ Desktop Development workload
- **CMake 3.20+** (download from cmake.org or use Visual Studio's bundled version)

### Build with Visual Studio

```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Or open the folder in Visual Studio and use the built-in CMake support.

---

## 🐧 Linux Setup

### Prerequisites (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev
sudo apt install libxcursor-dev libxi-dev libopenal-dev
```

### Build

```bash
git clone git@github.com:satrajitghosh183/Theisis_test.git
cd Theisis_test
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

---

## 📁 Project Structure

```
GameEngine/
├── Assets/           # Game assets (textures, meshes, audio, scenes)
├── Editor/           # Game Engine Editor application
├── Engine/           # Core engine library
│   ├── Animation/    # Animation system
│   ├── Audio/        # 3D audio (OpenAL)
│   ├── Core/         # Core systems (ECS, events, input)
│   ├── Graphics/     # Rendering (OpenGL, shaders)
│   ├── Physics/      # Physics simulation
│   ├── Scene/        # Scene management
│   └── Scripting/    # Lua scripting integration
├── Examples/         # Example applications
├── External/         # Third-party libraries (included)
│   ├── assimp/       # 3D model loading
│   ├── glad/         # OpenGL loader
│   ├── glfw/         # Window/input
│   ├── glm/          # Math library
│   ├── imgui/        # UI library
│   ├── lua/          # Scripting
│   ├── nlohmann_json/# JSON parsing
│   └── openal-soft/  # Audio
├── Tests/            # Unit tests
└── Tools/            # Utility scripts
```

---

## 🔧 CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_EXAMPLES` | ON | Build example applications |
| `BUILD_EDITOR` | ON | Build the GameEngine Editor |
| `BUILD_TESTS` | OFF | Build unit tests |
| `BUILD_SHARED_LIBS` | OFF | Build shared libraries |

Example with custom options:

```bash
cmake .. -DBUILD_TESTS=ON -DBUILD_EXAMPLES=OFF
```

---

## 🐛 Troubleshooting

### macOS: OpenGL Deprecation Warnings
macOS has deprecated OpenGL but it still works. You can suppress warnings:
```bash
cmake .. -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations"
```

### Linux: Missing OpenGL
```bash
sudo apt install libgl1-mesa-dev
```

### Windows: CMake can't find compiler
Make sure Visual Studio C++ workload is installed, or use:
```cmd
cmake -G "Visual Studio 17 2022" ..
```

---

## 📚 Documentation

See `Documentation/` directory for:
- API Reference
- Architecture Overview
- User Manual

---

## 📝 License

MIT License - see LICENSE file.
