cmake_minimum_required(VERSION 3.16)
project(VerletXEditor VERSION 1.0.0 LANGUAGES C CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(EDITOR_ENABLE_PROFILING "Enable profiling features" ON)

# Find packages
find_package(OpenGL REQUIRED)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Check if GLFW is available locally or fetch it
if(EXISTS "${CMAKE_SOURCE_DIR}/../external/glfw")
    add_subdirectory(${CMAKE_SOURCE_DIR}/../external/glfw ${CMAKE_BINARY_DIR}/glfw)
else()
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)
endif()

# GLM (header-only math library)
set(GLM_DIR ${CMAKE_SOURCE_DIR}/../cloth-simulation/third-party/include)

# ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/../external/imgui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

set(IMGUI_HEADERS
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imconfig.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imstb_rectpack.h
    ${IMGUI_DIR}/imstb_textedit.h
    ${IMGUI_DIR}/imstb_truetype.h
    ${IMGUI_DIR}/backends/imgui_impl_glfw.h
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.h
)

# GLAD sources
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/../external/glad)
set(GLAD_SOURCES ${GLAD_DIR}/src/glad.c)
set(GLAD_INCLUDE ${GLAD_DIR}/include)

# Editor sources
set(EDITOR_SOURCES
    main.cpp
    core/EditorApp.cpp
    core/EditorTheme.cpp
    core/SceneRenderer.cpp
    panels/SceneHierarchyPanel.cpp
    panels/InspectorPanel.cpp
    panels/ViewportPanel.cpp
    panels/ConsolePanel.cpp
    panels/AssetBrowserPanel.cpp
    panels/PhysicsPanel.cpp
    panels/PerformancePanel.cpp
)

set(EDITOR_HEADERS
    core/EditorApp.hpp
    core/EditorTheme.hpp
    core/SceneRenderer.hpp
    panels/EditorPanel.hpp
    panels/SceneHierarchyPanel.hpp
    panels/InspectorPanel.hpp
    panels/ViewportPanel.hpp
    panels/ConsolePanel.hpp
    panels/AssetBrowserPanel.hpp
    panels/PhysicsPanel.hpp
    panels/PerformancePanel.hpp
)

# Create ImGui library
add_library(imgui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Create GLAD library
add_library(glad STATIC ${GLAD_SOURCES})
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

# Create editor executable
add_executable(VerletXEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_include_directories(VerletXEditor PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/..
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLAD_DIR}/include
    ${GLM_DIR}
)

target_link_libraries(VerletXEditor PRIVATE
    imgui
    glad
    glfw
    OpenGL::GL
)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(VerletXEditor PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    # Link additional Windows libraries
    target_link_libraries(VerletXEditor PRIVATE dwmapi)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(VerletXEditor PRIVATE dl pthread)
elseif(APPLE)
    target_link_libraries(VerletXEditor PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(VerletXEditor PRIVATE /W4)
else()
    target_compile_options(VerletXEditor PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy resources to build directory
if(EXISTS ${CMAKE_SOURCE_DIR}/resources)
    add_custom_command(TARGET VerletXEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:VerletXEditor>/resources
    )
endif()

# Installation
install(TARGETS VerletXEditor
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/
    DESTINATION share/VerletXEditor/resources
    OPTIONAL
)

# Print configuration
message(STATUS "========================================")
message(STATUS "VerletX Engine Editor Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  ImGui Directory: ${IMGUI_DIR}")
message(STATUS "  GLM Directory: ${GLM_DIR}")
message(STATUS "========================================")
