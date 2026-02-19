
cmake_minimum_required(VERSION 3.10)
project(godot_like_editor)

set(CMAKE_CXX_STANDARD 17)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# ImGui source files
set(IMGUI_SOURCES
    extern/imgui/imgui.cpp
    extern/imgui/imgui_demo.cpp
    extern/imgui/imgui_draw.cpp
    extern/imgui/imgui_tables.cpp
    extern/imgui/imgui_widgets.cpp
    extern/imgui/backends/imgui_impl_glfw.cpp
    extern/imgui/backends/imgui_impl_opengl3.cpp
)

# Include directories
include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    extern/imgui
    extern/json
)

# Add executable
add_executable(godot_editor main2.cpp ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(godot_editor
    ${OPENGL_LIBRARIES}
    glfw
    ${GLEW_LIBRARIES}
    dl
    pthread
    X11
    Xrandr
    Xi
)
