cmake_minimum_required(VERSION 3.22)
project(
    robosim
    VERSION 1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Set up Eigen (using the bundled version) - FIXED PATH
set(EIGEN3_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib)
# Add Eigen as an interface library
add_library(Eigen3 INTERFACE)
target_include_directories(Eigen3 INTERFACE ${EIGEN3_INCLUDE_DIR})

# Set up GLEW (using the bundled version)
set(GLEW_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/lib/GLEW/include)
if(WIN32)
    set(GLEW_LIBRARIES ${PROJECT_SOURCE_DIR}/lib/GLEW/lib/Release/x64/glew32.lib)
else()
    # For Linux, you might need to install GLEW or build it
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLEW REQUIRED glew)
endif()

# Set up GLFW (using the bundled version)
set(GLFW_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/lib/GLFW/include)
if(WIN32)
    set(GLFW_LIBRARIES ${PROJECT_SOURCE_DIR}/lib/GLFW/lib-vc2022/glfw3.lib)
else()
    # For Linux, try to find system GLFW
    find_package(glfw3 REQUIRED)
    set(GLFW_LIBRARIES glfw)
endif()

# Build imgui from source
set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/lib/imgui/imgui)
set(IMGUI_SOURCES 
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

add_library(imgui ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_INCLUDE_DIRS}
)
target_link_libraries(imgui PUBLIC ${GLFW_LIBRARIES} OpenGL::GL)
if(UNIX)
    target_link_libraries(imgui PUBLIC ${GLEW_LIBRARIES})
endif()

# Build ImGuizmo
add_library(ImGuizmo ${PROJECT_SOURCE_DIR}/lib/ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo PUBLIC 
    ${PROJECT_SOURCE_DIR}/lib/ImGuizmo
    ${IMGUI_DIR}
)
target_link_libraries(ImGuizmo PUBLIC imgui)

# Project sources
set(SOURCES
    src/core/Application.cpp
    src/core/DhParam.cpp
    src/core/InverseKinematics.cpp
    src/core/JointedLink.cpp
    src/core/PidController.cpp
    src/core/Robot.cpp
    src/Main.cpp
    src/meshes/BoxMesh.cpp
    src/meshes/CylinderMesh.cpp
    src/meshes/JointedLinkMesh.cpp
    src/meshes/Mesh.cpp
    src/meshes/SphereMesh.cpp
    src/rendering/Camera.cpp 
)

# Include directories - FIXED TO USE lib/ for Eigen
include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/lib
    ${GLEW_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${IMGUI_DIR}
    ${PROJECT_SOURCE_DIR}/lib/ImGuizmo
)

# Create the executable
add_executable(robosim ${SOURCES})

# Link libraries
target_link_libraries(robosim PRIVATE
    OpenGL::GL
    imgui
    ImGuizmo
    Eigen3
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(robosim PRIVATE
        ${GLFW_LIBRARIES}
        ${GLEW_LIBRARIES}
    )
else()
    target_link_libraries(robosim PRIVATE
        ${GLFW_LIBRARIES}
        ${GLEW_LIBRARIES}
    )
endif()

# Set output directory
set_target_properties(robosim PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)