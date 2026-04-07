#!/usr/bin/env python3
"""
GameEngine Project Generator
Creates a new game project with proper structure
"""

import os
import sys
import argparse
from pathlib import Path

TEMPLATE_MAIN = """#include <GameEngine/Core/Application.hpp>
#include <GameEngine/Scene/Scene.hpp>

using namespace GameEngine;

class {ProjectName}App : public Application {{
public:
    {ProjectName}App() : Application("{ProjectName}") {{}}
    
protected:
    void OnInit() override {{
        GE_INFO("{ProjectName} initialized!");
        
        m_Scene = CreateRef<Scene>("{ProjectName}Scene");
        
        // TODO: Initialize your game here
    }}
    
    void OnUpdate(float deltaTime) override {{
        if (m_Scene)
            m_Scene->OnUpdate(deltaTime);
    }}
    
    void OnRender() override {{
        RenderCommand::SetClearColor({{0.1f, 0.1f, 0.1f, 1.0f}});
        RenderCommand::Clear();
        
        if (m_Scene) {{
            // TODO: Render your scene
        }}
    }}
    
private:
    Ref<Scene> m_Scene;
}};

GameEngine::Application* GameEngine::CreateApplication() {{
    return new {ProjectName}App();
}}

int main() {{
    auto app = GameEngine::CreateApplication();
    app->Run();
    delete app;
    return 0;
}}
"""

TEMPLATE_CMAKE = """cmake_minimum_required(VERSION 3.20)
project({ProjectName})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find GameEngine
find_package(GameEngine REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES "Source/*.cpp" "Source/*.hpp")

# Executable
add_executable({ProjectName} ${{SOURCES}})

# Link GameEngine
target_link_libraries({ProjectName} PRIVATE GameEngine::GameEngine)

# Copy assets to build directory
file(COPY Assets DESTINATION ${{CMAKE_BINARY_DIR}})
"""

TEMPLATE_GITIGNORE = """# Build
build/
bin/
lib/
*.exe
*.dll
*.so
*.dylib
*.a

# IDE
.vscode/
.vs/
.idea/
*.user
*.suo
*.sln
*.vcxproj*

# OS
.DS_Store
Thumbs.db
"""

def create_project(name, path):
    """Create a new game project"""
    project_path = Path(path) / name
    
    # Create directories
    directories = [
        project_path,
        project_path / "Source",
        project_path / "Assets" / "Textures",
        project_path / "Assets" / "Meshes",
        project_path / "Assets" / "Shaders",
        project_path / "Assets" / "Audio",
        project_path / "Assets" / "Scenes",
    ]
    
    for directory in directories:
        directory.mkdir(parents=True, exist_ok=True)
        print(f"Created: {directory}")
    
    # Create main.cpp
    main_content = TEMPLATE_MAIN.replace("{ProjectName}", name)
    main_file = project_path / "Source" / "main.cpp"
    main_file.write_text(main_content)
    print(f"Created: {main_file}")
    
    # Create CMakeLists.txt
    cmake_content = TEMPLATE_CMAKE.replace("{ProjectName}", name)
    cmake_file = project_path / "CMakeLists.txt"
    cmake_file.write_text(cmake_content)
    print(f"Created: {cmake_file}")
    
    # Create .gitignore
    gitignore_file = project_path / ".gitignore"
    gitignore_file.write_text(TEMPLATE_GITIGNORE)
    print(f"Created: {gitignore_file}")
    
    # Create README
    readme_content = f"""# {name}

Game created with GameEngine.

## Building
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Running
```bash
./bin/{name}
```
"""
    readme_file = project_path / "README.md"
    readme_file.write_text(readme_content)
    print(f"Created: {readme_file}")
    
    print(f"\nProject '{name}' created successfully at {project_path}")
    print(f"\nNext steps:")
    print(f"  cd {project_path}")
    print(f"  mkdir build && cd build")
    print(f"  cmake ..")
    print(f"  cmake --build .")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Create a new GameEngine project")
    parser.add_argument("name", help="Project name")
    parser.add_argument("--path", default=".", help="Project path (default: current directory)")
    
    args = parser.parse_args()
    
    create_project(args.name, args.path)