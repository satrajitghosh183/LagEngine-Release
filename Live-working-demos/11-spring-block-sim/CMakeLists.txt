cmake_minimum_required(VERSION 3.20)
project(SpringBlockSim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── VulkanBase shared library ────────────────────────────────────────────────
# This pulls in Vulkan, GLFW, ImGui (Vulkan backend), and GLM transitively.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common
                 ${CMAKE_CURRENT_BINARY_DIR}/common)

# ── shaderc (runtime GLSL → SPIR-V compilation) ─────────────────────────────
find_package(Vulkan REQUIRED)
# shaderc ships with the Vulkan SDK
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

# ── Project Sources ──────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Physics.cpp
    src/SpringMesh.cpp
    src/Camera.cpp
    src/Renderer.cpp
    src/UI.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    $ENV{VULKAN_SDK}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    VulkanBase
    ${SHADERC_LIB}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLFW_INCLUDE_VULKAN
)

# ── Platform linking ─────────────────────────────────────────────────────────
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl pthread)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
