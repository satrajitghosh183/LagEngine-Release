# GameEngine Build Instructions

## WSL/Linux Build

### Prerequisites
- WSL2 with Ubuntu (or similar Linux distribution)
- CMake 3.20 or higher
- GCC/G++ compiler (C++17 support)
- OpenGL development libraries

### Setup Dependencies

First, install system dependencies:
```bash
sudo apt update
sudo apt install -y build-essential cmake git wget pkg-config
sudo apt install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

Then download external dependencies:
```bash
bash setup_dependencies.sh
```

### Build

Run the build script:
```bash
bash build_wsl.sh
```

Or manually:
```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
cmake --build . -j$(nproc)
```

### Run Examples

All examples are built in `build/bin/`:

```bash
./build/bin/HelloTriangle
./build/bin/PhysicsDemo
./build/bin/ClothSimulation
./build/bin/CharacterController
./build/bin/FluidSimulation
./build/bin/RobotArm
```

## Features Implemented

### ✅ Complete Examples
1. **HelloTriangle** - Basic rendering demo
2. **PhysicsDemo** - 3D physics simulation with rigid bodies
3. **ClothSimulation** - Soft body cloth physics
4. **CharacterController** - First/third person character controller
5. **FluidSimulation** - SPH (Smoothed Particle Hydrodynamics) fluid simulation
6. **RobotArm** - Inverse kinematics (FABRIK algorithm) demo

### ✅ Editor UI
- **SceneHierarchyPanel** - View and manage entities in the scene
- **Inspector Panel** - Edit component properties
- Full component editing support (Transform, MeshRenderer, Camera, RigidBody, etc.)

### ✅ Engine Features
- Entity-Component System (ECS)
- 3D Rendering with OpenGL
- Physics system (Rigid bodies, Soft bodies, Fluids)
- Scene management
- Camera system
- Input handling
- UI system (ImGui)
- Material and shader system

## Project Structure

```
GameEngine/
├── Engine/           # Core engine code
│   ├── Core/        # Application, Time, Logger
│   ├── Graphics/    # Rendering, Meshes, Materials, Shaders
│   ├── Physics/     # Physics world, Rigid bodies, Soft bodies, Fluids
│   ├── Scene/       # Scene management, Entities, Components
│   ├── Platform/    # Window, Input
│   └── UI/          # ImGui integration
├── Examples/        # Demo applications
├── Editor/          # Editor panels and tools
├── External/        # Third-party dependencies
└── build/           # Build output
```

## Controls

### PhysicsDemo
- WASD - Rotate camera
- Space - Spawn falling objects
- R - Reset scene

### CharacterController
- WASD - Move character
- Space - Jump
- Mouse - Look around
- V - Toggle first/third person
- ESC - Show/hide cursor

### FluidSimulation
- Space - Spawn particles
- R - Reset simulation
- Middle Mouse - Rotate camera

### RobotArm
- Arrow Keys - Move target XY
- Page Up/Down - Move target Z
- Middle Mouse - Rotate camera

## Troubleshooting

### Build Issues
- Make sure all dependencies are installed
- Check that CMake version is 3.20+
- Ensure OpenGL libraries are installed

### Runtime Issues
- If examples don't run, check that X11 forwarding is enabled in WSL
- For display issues, you may need to set `DISPLAY` environment variable

## Next Steps

To enhance the engine further:
- Add more physics constraints (joints, springs)
- Implement asset import system
- Add animation system integration
- Create a full editor application
- Add networking support
- Implement audio system integration

