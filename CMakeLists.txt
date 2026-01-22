cmake_minimum_required(VERSION 3.20)
project(GameEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build tests" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

message(STATUS "Configuring external dependencies...")
if(EXISTS "${CMAKE_SOURCE_DIR}/External/CMakeLists.txt")
    add_subdirectory(External)
else()
    message(FATAL_ERROR "External dependencies not found! Please download dependencies first.")
endif()

message(STATUS "Configuring GameEngine library...")
if(EXISTS "${CMAKE_SOURCE_DIR}/Engine/CMakeLists.txt")
    add_subdirectory(Engine)
else()
    message(WARNING "Engine directory not configured yet")
endif()

if(BUILD_EXAMPLES)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Examples/CMakeLists.txt")
        add_subdirectory(Examples)
    else()
        message(WARNING "Examples not configured yet")
    endif()
endif()

# Build Editor
option(BUILD_EDITOR "Build the GameEngine Editor" ON)
if(BUILD_EDITOR)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Editor/CMakeLists.txt")
        add_subdirectory(Editor)
    else()
        message(WARNING "Editor not configured yet")
    endif()
endif()

if(BUILD_TESTS)
    if(EXISTS "${CMAKE_SOURCE_DIR}/Tests/CMakeLists.txt")
        enable_testing()
        add_subdirectory(Tests)
    endif()
endif()

include(GNUInstallDirs)

if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE" AND EXISTS "${CMAKE_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_NAME "GameEngine")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_VENDOR "GameEngine")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
    include(CPack)
endif()

message(STATUS "GameEngine configuration complete!")
