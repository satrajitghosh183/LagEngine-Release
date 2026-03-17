# Tests CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Collect all test sources
file(GLOB_RECURSE TEST_SOURCES
    "*.cpp"
)

# Editor source files needed by tests (HotkeyTests)
set(EDITOR_TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/Editor/Core/Hotkeys.cpp
)

# Create test executable
add_executable(GameEngineTests ${TEST_SOURCES} ${EDITOR_TEST_SOURCES})

target_include_directories(GameEngineTests PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine
    ${CMAKE_SOURCE_DIR}/Editor
    ${CMAKE_SOURCE_DIR}/External/glm
    ${CMAKE_SOURCE_DIR}/External/nlohmann_json/include
    ${CMAKE_SOURCE_DIR}/External/lua/src
)

target_link_libraries(GameEngineTests PRIVATE
    GameEngine
    GTest::gtest
    GTest::gmock
)

# Set output directory
set_target_properties(GameEngineTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_definitions(GameEngineTests PRIVATE GLM_FORCE_CTOR_INIT)

include(GoogleTest)
gtest_discover_tests(GameEngineTests)
