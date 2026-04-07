cmake_minimum_required(VERSION 3.15)
project(DeferredRendering)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable CUDA
enable_language(CUDA)
set(CMAKE_CUDA_ARCHITECTURES 75 CACHE STRING "CUDA architectures")
set(CUDA_USE_STATIC_CUDA_RUNTIME OFF CACHE BOOL "Use shared CUDA runtime" FORCE)
find_package(CUDA REQUIRED)

# GLFW
find_package(glfw3 REQUIRED)

# OpenGL
find_package(OpenGL REQUIRED)

# GLM (header-only) - try system first, then FetchContent
find_package(glm QUIET)
if(NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp
        PATHS /usr/include /usr/local/include ${CMAKE_SOURCE_DIR}/third_party/glm)
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

# GLAD (we'll include it directly)
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/third_party/glad)
include_directories(${GLAD_DIR}/include)

# ImGui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
include_directories(${IMGUI_DIR})

# CUDA
include_directories(${CUDA_INCLUDE_DIRS})


# Source files
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
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

# Add GLAD source if it exists
if(EXISTS ${GLAD_DIR}/src/glad.c)
    list(APPEND SOURCES ${GLAD_DIR}/src/glad.c)
endif()

# CUDA source files
set(CUDA_SOURCES
    cuda/particles.cu
    cuda/ssao.cu
    cuda/postprocess.cu
)

# Create CUDA object library
cuda_add_library(cuda_kernels STATIC ${CUDA_SOURCES})
set_target_properties(cuda_kernels PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
    POSITION_INDEPENDENT_CODE ON
)

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME}
    glfw
    OpenGL::GL
    ${CUDA_LIBRARIES}
    cuda_kernels
)
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    ${CMAKE_SOURCE_DIR}
    ${GLM_INCLUDE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLAD_DIR}/include
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    GLFW_INCLUDE_NONE
)

# CUDA properties
set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_RESOLVE_DEVICE_SYMBOLS ON
)

# Link CUDA libraries
target_link_libraries(${PROJECT_NAME} ${CUDA_LIBRARIES})

# Copy shaders to build directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR})

