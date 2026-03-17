cmake_minimum_required(VERSION 3.20)
project(RL_Backend VERSION 1.0.0 LANGUAGES C CXX)

# RL_Backend - standalone project (not part of GameEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options
option(RL_BACKEND_BUILD_CUDA "Build with CUDA support" OFF)
if(RL_BACKEND_BUILD_CUDA)
  enable_language(CUDA)
  set(CMAKE_CUDA_STANDARD 17)
endif()
option(RL_BACKEND_BUILD_RL "Build Python RL bindings (pybind11)" ON)
option(RL_BACKEND_BUILD_TESTS "Build tests" ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Third-party (spdlog, Catch2, pybind11)
add_subdirectory(third_party)

# RL Demo Engine library
add_subdirectory(engine)

# Demos
add_subdirectory(demos)

# Python bindings (optional)
if(RL_BACKEND_BUILD_RL)
    add_subdirectory(bindings)
endif()

# Tests
if(RL_BACKEND_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "RL_Backend: CUDA=${RL_BACKEND_BUILD_CUDA}, RL=${RL_BACKEND_BUILD_RL}, Tests=${RL_BACKEND_BUILD_TESTS}")
