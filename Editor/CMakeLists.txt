# Editor CMakeLists.txt

# Collect all editor source files
file(GLOB_RECURSE EDITOR_SOURCES
    "*.cpp"
)

file(GLOB_RECURSE EDITOR_HEADERS
    "*.hpp"
)

# Create editor executable
add_executable(GameEngineEditor ${EDITOR_SOURCES} ${EDITOR_HEADERS})

# Include directories
target_include_directories(GameEngineEditor
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/External/imgui
        ${CMAKE_SOURCE_DIR}/External/imgui/backends
)

# Link dependencies
target_link_libraries(GameEngineEditor
    PRIVATE
        GameEngine
        imgui
        nlohmann_json::nlohmann_json
)

# Set output directory
set_target_properties(GameEngineEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install editor executable
install(TARGETS GameEngineEditor
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
