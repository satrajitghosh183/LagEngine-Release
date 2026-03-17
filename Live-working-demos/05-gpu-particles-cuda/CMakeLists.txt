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

include(FetchContent)

# GLFW
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11     ON  CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

file(GLOB SRC_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/*.cu
)

add_executable(gpu_particles
  ${SRC_FILES}
  src/shaders/particles.vert
  src/shaders/particles.frag
)

# RL Interface executable
add_executable(gpu_particles_rl
  rl_interface/main_rl.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gl_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/sim.cu
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cuda/interop.cu
  src/shaders/particles.vert
  src/shaders/particles.frag
)

# Vendored GLAD
add_library(gladlib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c)
target_include_directories(gladlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include)

target_include_directories(gpu_particles PRIVATE
  ${glfw_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gpu_particles PRIVATE glfw gladlib OpenGL::GL)

target_include_directories(gpu_particles_rl PRIVATE
  ${glfw_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(gpu_particles_rl PRIVATE glfw gladlib OpenGL::GL)

if(UNIX AND NOT APPLE)
  target_link_libraries(gpu_particles PRIVATE dl pthread)
  target_link_libraries(gpu_particles_rl PRIVATE dl pthread)
endif()

# Copy shaders to build dir
add_custom_command(TARGET gpu_particles POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/particles.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/particles.frag
    $<TARGET_FILE_DIR:gpu_particles>)

add_custom_command(TARGET gpu_particles_rl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/particles.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/particles.frag
    $<TARGET_FILE_DIR:gpu_particles_rl>)

if(MSVC)
  target_compile_options(gpu_particles PRIVATE /W4)
  target_compile_options(gpu_particles_rl PRIVATE /W4)
else()
  target_compile_options(gpu_particles PRIVATE -Wall -Wextra -Wno-unused-parameter)
  target_compile_options(gpu_particles_rl PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

target_compile_options(gpu_particles PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>)
target_compile_options(gpu_particles_rl PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--use_fast_math>)
