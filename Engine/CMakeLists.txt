# Collect all engine source files
file(GLOB_RECURSE ENGINE_SOURCES
    "Assets/*.cpp"
    "Build/*.cpp"
    "Compute/*.cpp"
    "Core/*.cpp"
    "Graphics/*.cpp"
    "Physics/*.cpp"
    "Scene/*.cpp"
    "Audio/*.cpp"
    "ParticleSystem/*.cpp"
    "Platform/*.cpp"
    "Resources/*.cpp"
    "Utilities/*.cpp"
    "Scripting/*.cpp"
    "Animation/*.cpp"
    "UI/*.cpp"
    "Robotics/*.cpp"
    "Navigation/*.cpp"
    "Networking/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "Assets/*.hpp"
    "Build/*.hpp"
    "Compute/*.hpp"
    "Core/*.hpp"
    "Graphics/*.hpp"
    "Physics/*.hpp"
    "Scene/*.hpp"
    "Audio/*.hpp"
    "ParticleSystem/*.hpp"
    "Platform/*.hpp"
    "Resources/*.hpp"
    "Utilities/*.hpp"
    "Scripting/*.hpp"
    "Animation/*.hpp"
    "UI/*.hpp"
    "Robotics/*.hpp"
    "Navigation/*.hpp"
    "Networking/*.hpp"
)

# Create engine library
add_library(GameEngine ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# Set target properties
set_target_properties(GameEngine PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${ENGINE_HEADERS}"
)

# Include directories
target_include_directories(GameEngine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# GLM configuration: must match test binary to avoid calling-convention ABI mismatch
# GLM_FORCE_CTOR_INIT makes glm types non-trivially-copyable (user-provided copy ctor),
# which changes glm::vec3/quat/etc. return convention from XMM registers to sret.
# Both the library and any consumer (Tests) must use the same setting.
target_compile_definitions(GameEngine PUBLIC GLM_FORCE_CTOR_INIT)

# GLM tweaks: radians-everywhere prevents accidental deg/rad conversions in
# hot paths, and 0..1 depth range matches Vulkan's clip space (vs GL's -1..1).
# GLM_FORCE_INTRINSICS is intentionally NOT set — it makes some glm::pi()-style
# helpers non-constexpr which breaks compile-time constants in MathUtils.hpp.
target_compile_definitions(GameEngine PUBLIC
    GLM_FORCE_RADIANS
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Apply fast-math + arch-native to the engine library only (PRIVATE).
# Consumers can still build with strict math without inheriting these flags.
if(GE_HOT_PATH_FLAGS)
    target_compile_options(GameEngine PRIVATE
        $<$<CONFIG:Release>:${GE_HOT_PATH_FLAGS}>
        $<$<CONFIG:RelWithDebInfo>:${GE_HOT_PATH_FLAGS}>
    )
endif()

# Vulkan rendering backend
target_compile_definitions(GameEngine PUBLIC GE_RENDERER_VULKAN)

# Link dependencies — Vulkan replaces GLAD/OpenGL
target_link_libraries(GameEngine
    PUBLIC
        glfw
        Vulkan::Vulkan
        glm
        nlohmann_json
        imgui
        lua
        stb_image
)

# VMA (Vulkan Memory Allocator)
if(TARGET GPUOpen::VulkanMemoryAllocator)
    target_link_libraries(GameEngine PUBLIC GPUOpen::VulkanMemoryAllocator)
elseif(TARGET VulkanMemoryAllocator)
    target_link_libraries(GameEngine PUBLIC VulkanMemoryAllocator)
else()
    # Fallback: include VMA header directory directly from FetchContent
    if(vma_SOURCE_DIR)
        target_include_directories(GameEngine PUBLIC ${vma_SOURCE_DIR}/include)
    endif()
endif()

# shaderc for runtime GLSL -> SPIR-V compilation
if(TARGET Vulkan::shaderc_combined)
    target_link_libraries(GameEngine PRIVATE Vulkan::shaderc_combined)
    target_compile_definitions(GameEngine PRIVATE GE_HAS_SHADERC)
elseif(TARGET shaderc_combined)
    target_link_libraries(GameEngine PRIVATE shaderc_combined)
    target_compile_definitions(GameEngine PRIVATE GE_HAS_SHADERC)
endif()

# Link Assimp for asset importing
if(TARGET assimp)
    target_link_libraries(GameEngine PRIVATE assimp)
    target_compile_definitions(GameEngine PRIVATE GE_HAS_ASSIMP)
endif()

# Link OpenAL for audio
if(TARGET OpenAL)
    target_link_libraries(GameEngine PRIVATE OpenAL::OpenAL)
    target_compile_definitions(GameEngine PRIVATE GE_HAS_OPENAL)
elseif(TARGET alsoft)
    target_link_libraries(GameEngine PRIVATE alsoft)
    target_compile_definitions(GameEngine PRIVATE GE_HAS_OPENAL)
endif()

# CUDA compute integration (optional)
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        target_link_libraries(GameEngine PRIVATE CUDA::cudart)
        target_compile_definitions(GameEngine PUBLIC GE_HAS_CUDA)
        target_include_directories(GameEngine PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
        message(STATUS "CUDA support enabled: ${CUDAToolkit_VERSION}")
    endif()
else()
    message(STATUS "CUDA not found — building without GPU compute support")
endif()

# Platform-specific libraries (no more OpenGL)
if(WIN32)
    target_link_libraries(GameEngine PRIVATE winhttp ws2_32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(GameEngine PRIVATE X11 pthread dl)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)

    target_link_libraries(GameEngine PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
    )
endif()

# Install library (no export - external deps not in export sets)
install(TARGETS GameEngine
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine
)
