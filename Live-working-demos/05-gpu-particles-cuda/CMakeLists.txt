cmake_minimum_required(VERSION 3.22)
project(GPUParticlesCUDA LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_language(CUDA)
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES 75)
endif()
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# ── VulkanBase shared library (pulls in Vulkan, GLFW, ImGui, GLM) ──────────
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common
                 ${CMAKE_CURRENT_BINARY_DIR}/common)

# ── Vulkan SDK & shaderc (runtime GLSL -> SPIR-V) ─────────────────────────
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

# ── Main executable ────────────────────────────────────────────────────────
set(SRC_FILES
    src/main.cpp
    src/renderer.cpp
    src/cuda/sim.cu
    src/cuda/interop.cu
)

add_executable(gpu_particles ${SRC_FILES})

target_include_directories(gpu_particles PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $ENV{VULKAN_SDK}/include
)

target_link_libraries(gpu_particles PRIVATE
    VulkanBase
    ${SHADERC_LIB}
)

target_compile_definitions(gpu_particles PRIVATE
    GLFW_INCLUDE_VULKAN
)

# ── RL Interface executable ────────────────────────────────────────────────
add_executable(gpu_particles_rl
    rl_interface/main_rl.cpp
    src/renderer.cpp
    src/cuda/sim.cu
    src/cuda/interop.cu
)

target_include_directories(gpu_particles_rl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    $ENV{VULKAN_SDK}/include
)

target_link_libraries(gpu_particles_rl PRIVATE
    VulkanBase
    ${SHADERC_LIB}
)

target_compile_definitions(gpu_particles_rl PRIVATE
    GLFW_INCLUDE_VULKAN
)

# ── Platform linking ───────────────────────────────────────────────────────
if(WIN32)
    target_compile_definitions(gpu_particles PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_compile_definitions(gpu_particles_rl PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(gpu_particles PRIVATE dl pthread)
    target_link_libraries(gpu_particles_rl PRIVATE dl pthread)
endif()

# ── Compiler options ──────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(gpu_particles PRIVATE /W4)
    target_compile_options(gpu_particles_rl PRIVATE /W4)
else()
    target_compile_options(gpu_particles PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_options(gpu_particles_rl PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_compile_options(gpu_particles PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>)
target_compile_options(gpu_particles_rl PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>)
