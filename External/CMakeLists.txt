# External Dependencies

# GLFW (used for windowing with both OpenGL and Vulkan)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# GLM (header-only)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# Vulkan SDK
find_package(Vulkan REQUIRED)
message(STATUS "Vulkan found: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan libraries: ${Vulkan_LIBRARIES}")

# Vulkan::Vulkan is created at root CMakeLists.txt scope.
# This block is intentionally empty — find_package was hoisted up.

# Vulkan Memory Allocator (VMA) - fetched via FetchContent
include(FetchContent)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(vma)

# shaderc for runtime GLSL -> SPIR-V compilation
# Use the one bundled with the Vulkan SDK if available
if(Vulkan_shaderc_combined_FOUND OR TARGET Vulkan::shaderc_combined)
    message(STATUS "Using Vulkan SDK shaderc")
else()
    # Fallback: look for system shaderc
    find_library(SHADERC_LIB shaderc_combined PATHS
        $ENV{VULKAN_SDK}/lib
        $ENV{VULKAN_SDK}/Lib
    )
    if(SHADERC_LIB)
        add_library(shaderc_combined STATIC IMPORTED)
        set_target_properties(shaderc_combined PROPERTIES
            IMPORTED_LOCATION ${SHADERC_LIB}
        )
        message(STATUS "Found shaderc: ${SHADERC_LIB}")
    else()
        message(WARNING "shaderc not found - runtime shader compilation will not be available")
    endif()
endif()

# ImGui (with Vulkan + GLFW backends)
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_vulkan.cpp
)
target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
)
target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

# nlohmann_json (header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE nlohmann_json/include)
add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)

# stb_image
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE stb_image)

# Assimp (enabled for Linux/WSL)
if(UNIX OR WIN32)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(assimp)
endif()

# Lua
add_library(lua STATIC
    lua/src/lapi.c
    lua/src/lauxlib.c
    lua/src/lbaselib.c
    lua/src/lcode.c
    lua/src/lcorolib.c
    lua/src/lctype.c
    lua/src/ldblib.c
    lua/src/ldebug.c
    lua/src/ldo.c
    lua/src/ldump.c
    lua/src/lfunc.c
    lua/src/lgc.c
    lua/src/linit.c
    lua/src/liolib.c
    lua/src/llex.c
    lua/src/lmathlib.c
    lua/src/lmem.c
    lua/src/loadlib.c
    lua/src/lobject.c
    lua/src/lopcodes.c
    lua/src/loslib.c
    lua/src/lparser.c
    lua/src/lstate.c
    lua/src/lstring.c
    lua/src/lstrlib.c
    lua/src/ltable.c
    lua/src/ltablib.c
    lua/src/ltm.c
    lua/src/lundump.c
    lua/src/lutf8lib.c
    lua/src/lvm.c
    lua/src/lzio.c
)
target_include_directories(lua PUBLIC lua/src)
if(UNIX)
    target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
    target_link_libraries(lua PUBLIC m dl)
endif()

# OpenAL Soft
if(UNIX OR WIN32)
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
    add_subdirectory(openal-soft)
endif()
