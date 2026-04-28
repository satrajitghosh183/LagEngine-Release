cmake_minimum_required(VERSION 3.20)
project(GameEngine VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build tests" ON)
option(GE_ENABLE_LTO "Enable link-time optimization in release builds" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to RelWithDebInfo when no build type is specified — picks up the
# -O2 + -g optimization profile so the engine and editor run smoothly.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
        "Default build type" FORCE)
endif()

# LTO: ~10-30% speedup on hot rendering / physics paths via cross-TU inlining.
# Skip on Debug to keep build times sane and stack traces accurate.
if(GE_ENABLE_LTO AND
   (CMAKE_BUILD_TYPE STREQUAL "Release" OR
    CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR
    CMAKE_BUILD_TYPE STREQUAL "MinSizeRel"))
    include(CheckIPOSupported)
    check_ipo_supported(RESULT GE_LTO_SUPPORTED OUTPUT GE_LTO_ERROR)
    if(GE_LTO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
        message(STATUS "LTO enabled for ${CMAKE_BUILD_TYPE}")
    else()
        message(STATUS "LTO requested but not supported: ${GE_LTO_ERROR}")
    endif()
endif()

# Hot-path math: enable fast-math for the GameEngine target only (set later).
if(NOT MSVC)
    set(GE_HOT_PATH_FLAGS -ffast-math -fno-math-errno)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Find Vulkan at root scope so the Vulkan::Vulkan imported target is
# unambiguously global for every subdirectory (some FindVulkan modules
# only register the target in the calling directory's scope).
find_package(Vulkan REQUIRED)
if(NOT TARGET Vulkan::Vulkan)
    add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${Vulkan_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Vulkan_INCLUDE_DIRS}"
    )
endif()

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

# Install shaders and content for runtime (editor/examples resolve paths via RuntimePaths)
if(EXISTS "${CMAKE_SOURCE_DIR}/Assets/Shaders")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/Assets/Shaders/"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/GameEngine/Shaders
        FILES_MATCHING PATTERN "*.vert" PATTERN "*.frag" PATTERN "*.glsl"
    )
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/Content")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/Content/"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/GameEngine/Content
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE" AND EXISTS "${CMAKE_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_NAME "GameEngine")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_VENDOR "GameEngine")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GameEngine - Educational 3D Game Engine")

    # Platform-specific generators
    if(WIN32)
        set(CPACK_GENERATOR "NSIS;ZIP")
        set(CPACK_NSIS_PACKAGE_NAME "GameEngine")
        set(CPACK_NSIS_DISPLAY_NAME "GameEngine ${PROJECT_VERSION}")
        set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\GameEngine Editor.lnk' '$INSTDIR\\\\bin\\\\GameEngineEditor.exe'")
        set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\GameEngine Editor.lnk'")
    elseif(APPLE)
        set(CPACK_GENERATOR "DragNDrop;ZIP")
        set(CPACK_DMG_VOLUME_NAME "GameEngine")
    else()
        set(CPACK_GENERATOR "DEB;TGZ")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "GameEngine")
        set(CPACK_DEBIAN_FILE_NAME "gameengine_${PROJECT_VERSION}_amd64.deb")
    endif()

    include(CPack)
endif()

message(STATUS "GameEngine configuration complete!")
