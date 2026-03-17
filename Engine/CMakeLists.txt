# Collect all engine source files
file(GLOB_RECURSE ENGINE_SOURCES
    "Assets/*.cpp"
    "Build/*.cpp"
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
)

file(GLOB_RECURSE ENGINE_HEADERS
    "Assets/*.hpp"
    "Build/*.hpp"
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

# Link dependencies
target_link_libraries(GameEngine
    PUBLIC
        glfw
        glad
        glm
        nlohmann_json
        imgui
        lua
    PRIVATE
        stb_image
)

# Optional: Link ImGui if available (for backward compatibility, but Editor should link it)
# Editor target will link imgui separately
if(TARGET imgui)
    # Only link if explicitly requested (for backward compatibility)
    # target_link_libraries(GameEngine PUBLIC imgui)
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

# Platform-specific libraries
if(WIN32)
    target_link_libraries(GameEngine PRIVATE opengl32 winhttp)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(GameEngine PRIVATE GL X11 pthread dl)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    
    target_link_libraries(GameEngine PRIVATE
        ${COCOA_LIBRARY}
        ${IOKIT_LIBRARY}
        ${COREVIDEO_LIBRARY}
        ${OPENGL_LIBRARY}
    )
endif()

# Install library (no export - external deps not in export sets)
install(TARGETS GameEngine
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GameEngine
)