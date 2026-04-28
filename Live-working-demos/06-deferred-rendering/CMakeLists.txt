cmake_minimum_required(VERSION 3.20)
project(DeferredRendering LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CUDA
set(CMAKE_CUDA_ARCHITECTURES 75 CACHE STRING "CUDA architectures")
set(CUDA_USE_STATIC_CUDA_RUNTIME OFF CACHE BOOL "Use shared CUDA runtime" FORCE)
find_package(CUDAToolkit REQUIRED)

# Vulkan (with shaderc for runtime GLSL->SPIR-V)
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# GLFW
find_package(glfw3 REQUIRED)

# GLM (header-only) - try system first, then FetchContent
find_package(glm QUIET)
if(NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATHS /usr/include /usr/local/include
              ${CMAKE_SOURCE_DIR}/../../External/glm
              ${CMAKE_SOURCE_DIR}/third_party/glm)
    if(GLM_INCLUDE_DIR)
        include_directories(${GLM_INCLUDE_DIR})
    else()
        include(FetchContent)
        FetchContent_Declare(glm
            GIT_REPOSITORY https://github.com/g-truc/glm.git
            GIT_TAG 1.0.1
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(glm)
    endif()
else()
    include_directories(${GLM_INCLUDE_DIRS})
endif()

# VulkanBase shared library from ../common/
add_subdirectory(${CMAKE_SOURCE_DIR}/../common ${CMAKE_BINARY_DIR}/VulkanBase)

# Source files (Vulkan only -- no GLAD, no GL)
set(SOURCES
    src/main.cpp
    src/Window.cpp
    src/RenderGraph.cpp
    src/RenderTask.cpp
    src/DAGScheduler.cpp
    src/PipelineController.cpp
    src/AIBackend.cpp
    src/DemoScene.cpp
    src/Camera.cpp
    src/Shader.cpp
    src/Framebuffer.cpp
    src/Mesh.cpp
    src/ParticleSystem.cpp
    src/SSAO.cpp
    src/ShadowMap.cpp
    src/GBuffer.cpp
    src/Profiler.cpp
    src/UI.cpp
)

# CUDA source files
set(CUDA_SOURCES
    cuda/particles.cu
    cuda/ssao.cu
    cuda/postprocess.cu
)

# Create CUDA object library
add_library(cuda_kernels STATIC ${CUDA_SOURCES})
set_target_properties(cuda_kernels PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    POSITION_INDEPENDENT_CODE ON
)
target_link_libraries(cuda_kernels PUBLIC CUDA::cudart)

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    VulkanBase
    Vulkan::Vulkan
    Vulkan::shaderc_combined
    glfw
    CUDA::cudart
    cuda_kernels
)
target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_SHADERC=1)

target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/../common
    ${CUDAToolkit_INCLUDE_DIRS}
)

# CUDA properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})
