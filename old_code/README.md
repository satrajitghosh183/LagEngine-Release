# LAG Engine

## 🚀 Overview

LAG Engine is a custom-built game development framework designed from first principles, focused on:

- **Dual-dimension architecture** supporting both 2D and 3D physics simulations
- **Realistic physics simulation** using Verlet integration
- **Advanced cloth dynamics** with tearing and deformation
- **Modular component-based design** for extensibility
- **Optimized rendering pipeline** using modern OpenGL
- **Foundation for AI integration** and complex agent behaviors

Built as an academic research project, LAG Engine demonstrates how fundamental simulation concepts can be implemented without relying on existing game engines, providing unparalleled control and flexibility.

---

## ✅ Completed Features

### Physics Systems
- [x] **Verlet-based physics integration** for stability
- [x] **Cloth simulation** with constraint-based dynamics
- [x] **Dynamic tearing** governed by stress thresholds
- [x] **Projectile physics** with proper momentum transfer
- [x] **Particle systems** for position-based dynamics

### Rendering Systems
- [x] **Custom shader framework** for material definition
- [x] **2D/3D mesh rendering** with appropriate abstractions
- [x] **Dynamic mesh generation** for deformable objects
- [x] **Texture mapping** and material systems
- [x] **Phong lighting model** with ambient, diffuse, and specular components

### Architecture
- [x] **Modular scene management** with object lifecycles
- [x] **Unified physics core** shared between 2D and 3D contexts
- [x] **Event-based input handling** for interactive elements
- [x] **Memory-safe object management** system

---

## 🔭 Roadmap

### 🧩 Core Architecture
- [ ] Full **Entity-Component System** implementation
- [ ] **Memory pool allocators** for physics objects
- [ ] **Serialization system** for scene storage and loading
- [ ] **Event propagation** framework for inter-object communication

### 🌊 Advanced Physics
- [ ] **Fluid simulation** using SPH (Smoothed Particle Hydrodynamics)
- [ ] **Soft body physics** for deformable objects
- [ ] **Fracturing system** for realistic destruction
- [ ] **Constraint solver optimization** via parallel processing
- [ ] **Joint systems** for articulated bodies

### 🎨 Graphics Enhancements
- [ ] **PBR materials** (Physically-Based Rendering)
- [ ] **Shadow mapping** for realistic lighting
- [ ] **Ambient occlusion** and global illumination approximation
- [ ] **Post-processing pipeline** for visual effects
- [ ] **Particle system renderer** for effects like fire, smoke, etc.

### 🧠 AI Integration
- [ ] **Behavior trees** for complex agent decision-making
- [ ] **Pathfinding** using A* and navigation meshes
- [ ] **LLM integration** for dynamic content generation
- [ ] **Reinforcement learning** for adaptive behaviors
- [ ] **Agent perception systems** for realistic AI responses

### 🛠️ Editor & Tools
- [ ] **Full-featured editor UI** replacing temporary ImGui implementation
- [ ] **Visual node editor** for material and behavior creation
- [ ] **Scene hierarchy management** with drag-and-drop
- [ ] **Real-time physics debugging** visualizations
- [ ] **Asset management system** for resources

---

## 📦 Dependencies

### Core Dependencies
- **C++17 or higher** - For modern language features
- **CMake 3.16+** - Build system

### Graphics
- **OpenGL 3.3+** - Core rendering API
- **GLAD** - OpenGL loader
- **GLFW** - Windowing and input (3D mode)
- **SFML 2.5+** - Windowing, input, and 2D rendering
- **stb_image** - Image loading (textures)
- **glm** - Mathematics library for 3D operations

### UI & Debugging
- **Dear ImGui** - Debugging interface and temporary tools
- **ImGui-SFML** - ImGui integration with SFML

### Physics (Custom Implementations)
- **Verlet Physics** - Custom implementation, no external dependency
- **Constraint Solver** - Custom implementation for cloth physics

### Optional Dependencies
- **Assimp** - For 3D model loading (future)
- **Bullet Physics** - For advanced collision detection (future)
- **OpenAL** - For audio support (future)

---

## 🛠️ Build Instructions

### Prerequisites
Ensure you have installed:
- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 19.14+)
- CMake 3.16 or higher
- SFML 2.5 development libraries
- OpenGL development libraries

### Option 1: Using the Build Script (Recommended)
The easiest way to build LAG Engine is using the provided build script:

```bash
# Clone the repository
git clone https://github.com/satrajitghosh183/LAGEngine.git
cd LAGEngine

# Make the build script executable
chmod +x build_game_engine.sh

# Run the build script
./build_game_engine.sh
```

This script will:
- Create and clean the build directory
- Run CMake to configure the project
- Build the project using the appropriate system commands
- Report any errors if they occur

### Option 2: Manual Build

#### Linux/macOS
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake libsfml-dev libglm-dev libglfw3-dev libgl1-mesa-dev

# Clone repository
git clone https://github.com/satrajitghosh183/LAGEngine.git
cd LAGEngine

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make
```

#### Windows
```bash
# Install dependencies via vcpkg
vcpkg install sfml glm glfw3

# Clone repository
git clone https://github.com/satrajitghosh183/LAGEngine.git
cd LAGEngine

# Create build directory
mkdir build
cd build

# Configure and build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

### Troubleshooting Common Build Issues

If you encounter build issues, try the following:

1. **SFML not found**: Ensure SFML development libraries are installed and findable by CMake
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libsfml-dev
   
   # macOS
   brew install sfml
   
   # Windows (vcpkg)
   vcpkg install sfml
   ```

2. **OpenGL issues**: Make sure OpenGL development libraries are installed
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libgl1-mesa-dev
   ```

3. **CMake version too old**: Update CMake to version 3.16 or newer
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake
   
   # macOS
   brew install cmake
   ```

4. **Compiler errors**: Ensure you're using a C++17 compatible compiler
   ```bash
   # Check GCC version
   gcc --version
   
   # Check Clang version
   clang --version
   ```

---

## 🎮 Running the Demos

The LAG Engine comes with several demonstration applications:

- **main2D** - 2D cloth simulation with interactive cannon projectiles
  ```bash
  ./build/main2D
  ```

- **main3D** - 3D cloth and ball physics with realistic lighting
  ```bash
  ./build/main3D
  ```

### Controls

#### 2D Demo
- **Left/Right Arrow** - Rotate cannon
- **Up/Down Arrow** - Adjust cannon power
- **Space** - Fire ball
- **R** - Respawn cloth

#### 3D Demo
- **W/A/S/D** - Move camera
- **Mouse** - Look around
- **Space** - Create new ball
- **R** - Reset simulation



## 🌟 Acknowledgments

This engine was developed as part of an academic research project in physical simulation and game engine architecture. Special thanks to:

- The SFML team for their excellent 2D graphics library
- The OpenGL community for resources and documentation
- The ImGui team for their immediate-mode GUI library
- Game Engine Architecture (Jason Gregory) for foundational concepts
- All contributors and testers who have helped shape this project
