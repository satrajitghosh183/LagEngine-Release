cmake_minimum_required(VERSION 3.20)
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

# Add the shared VulkanBase library
add_subdirectory(${CMAKE_SOURCE_DIR}/../common ${CMAKE_BINARY_DIR}/common)

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

# Create editor executable
add_executable(VerletXEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

target_include_directories(VerletXEditor PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(VerletXEditor PRIVATE
    VulkanBase
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
message(STATUS "  Backend: Vulkan (via VulkanBase)")
message(STATUS "========================================")
