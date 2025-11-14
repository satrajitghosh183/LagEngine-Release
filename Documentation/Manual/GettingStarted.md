# 🚀 Getting Started with GameEngine

## Installation

### Windows

1. Download `GameEngine-1.0.0-win64.exe` from [Releases](releases)
2. Run the installer
3. Add to PATH: `C:\Program Files\GameEngine\bin`

### Linux (Ubuntu/Debian)
```bash
wget https://github.com/yourusername/GameEngine/releases/download/v1.0.0/GameEngine-1.0.0-linux.deb
sudo dpkg -i GameEngine-1.0.0-linux.deb
```

### macOS
```bash
brew tap yourusername/gameengine
brew install gameengine
```

## Creating Your First Project

### Method 1: Using Project Generator (Recommended)
```bash
python -m gameengine create MyFirstGame
cd MyFirstGame
```

### Method 2: Manual Setup

1. **Create project structure:**
```
MyFirstGame/
├── CMakeLists.txt
├── Source/
│   └── main.cpp
└── Assets/
    ├── Textures/
    ├── Meshes/
    └── Shaders/
```

2. **CMakeLists.txt:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyFirstGame)

find_package(GameEngine REQUIRED)

add_executable(MyFirstGame Source/main.cpp)
target_link_libraries(MyFirstGame PRIVATE GameEngine::GameEngine)
```

3. **Source/main.cpp:**
```cpp
#include <GameEngine/Core/Application.hpp>

class MyGame : public GameEngine::Application {
public:
    MyGame() : Application("My First Game") {}
    
    void OnInit() override {
        GE_INFO("Game started!");
    }
    
    void OnUpdate(float deltaTime) override {
        // Game logic here
    }
    
    void OnRender() override {
        // Rendering here
    }
};

GameEngine::Application* GameEngine::CreateApplication() {
    return new MyGame();
}
```

## Building
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Running
```bash
./bin/MyFirstGame  # Linux/Mac
.\bin\MyFirstGame.exe  # Windows
```

## Next Steps

- 📖 Follow the [tutorials](Tutorials/)
- 🎓 Check out [examples](../../Examples/)
- 📘 Read the [API documentation](../API/)
- 💬 Join our [Discord](https://discord.gg/yourserver)