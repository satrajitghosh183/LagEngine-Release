cmake_minimum_required(VERSION 3.20)
project(
    robosim
    VERSION 1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── VulkanBase shared library ────────────────────────────────────────────────
# Pulls in Vulkan, GLFW, ImGui (Vulkan backend), and GLM transitively.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common
                 ${CMAKE_CURRENT_BINARY_DIR}/common)

# ── shaderc (runtime GLSL -> SPIR-V compilation) ────────────────────────────
find_package(Vulkan REQUIRED)
find_library(SHADERC_LIB shaderc_combined
    HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/Lib")
if(NOT SHADERC_LIB)
    find_library(SHADERC_LIB shaderc_shared
        HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/Lib")
endif()
if(NOT SHADERC_LIB)
    message(FATAL_ERROR "shaderc library not found. Install the Vulkan SDK.")
endif()
message(STATUS "shaderc: ${SHADERC_LIB}")

# Set up Eigen (using the bundled version)
set(EIGEN3_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/lib)
add_library(Eigen3 INTERFACE)
target_include_directories(Eigen3 INTERFACE ${EIGEN3_INCLUDE_DIR})

# Build ImGuizmo
set(IMGUI_DIR_LOCAL ${PROJECT_SOURCE_DIR}/lib/imgui/imgui)
add_library(ImGuizmo ${PROJECT_SOURCE_DIR}/lib/ImGuizmo/ImGuizmo.cpp)
target_include_directories(ImGuizmo PUBLIC
    ${PROJECT_SOURCE_DIR}/lib/ImGuizmo
    ${IMGUI_DIR_LOCAL}
)
# ImGuizmo needs imgui headers — VulkanBase already provides them via imgui_demo,
# but we also point at the local copy for the header search.
if(TARGET imgui_demo)
    target_link_libraries(ImGuizmo PUBLIC imgui_demo)
elseif(TARGET imgui)
    target_link_libraries(ImGuizmo PUBLIC imgui)
else()
    # Fallback: just use the include directory
    target_include_directories(ImGuizmo PUBLIC ${IMGUI_DIR_LOCAL})
endif()

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

# Create the executable
add_executable(robosim ${SOURCES})

target_include_directories(robosim PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/lib/ImGuizmo
    $ENV{VULKAN_SDK}/include
)

# Link libraries
target_link_libraries(robosim PRIVATE
    VulkanBase
    ${SHADERC_LIB}
    ImGuizmo
    Eigen3
)

target_compile_definitions(robosim PRIVATE
    GLFW_INCLUDE_VULKAN
)

# Platform-specific linking
if(WIN32)
    target_compile_definitions(robosim PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(robosim PRIVATE dl pthread)
elseif(APPLE)
    target_link_libraries(robosim PRIVATE
        "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()

if(MSVC)
    target_compile_options(robosim PRIVATE /W3)
else()
    target_compile_options(robosim PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Set output directory
set_target_properties(robosim PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
