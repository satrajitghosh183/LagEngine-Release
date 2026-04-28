# Examples CMakeLists.txt
#
# Examples are split into two groups:
#   * Vulkan-ready: build by default after the GL→Vulkan migration.
#   * Legacy-GL:    pre-Vulkan demos that make raw glXXX calls. Gated behind
#                   ENABLE_LEGACY_GL_EXAMPLES (default OFF) until rewritten.

option(ENABLE_LEGACY_GL_EXAMPLES "Build pre-Vulkan example apps that still use raw GL" OFF)

set(ENGINE_INCLUDE_DIR ${CMAKE_SOURCE_DIR})

# ─── Vulkan-ready examples ─────────────────────────────────────────────────

# Example 02: Physics Demo (compiles with backward-compat shims)
add_executable(PhysicsDemo 02_PhysicsDemo/PhysicsDemoApp.cpp)
target_link_libraries(PhysicsDemo PRIVATE GameEngine imgui)
target_include_directories(PhysicsDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

# Example 04: Character Controller
add_executable(CharacterController 04_CharacterController/CharacterControllerApp.cpp)
target_link_libraries(CharacterController PRIVATE GameEngine imgui)
target_include_directories(CharacterController PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

# Example 06: Robot Arm
add_executable(RobotArm 06_RobotArm/RobotArmApp.cpp)
target_link_libraries(RobotArm PRIVATE GameEngine imgui)
target_include_directories(RobotArm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

# Wicked Style Template
add_executable(WickedStyleTemplate
    WickedStyleTemplate/WickedStyleTemplateApp.cpp
    WickedStyleTemplate/WickedStyleTemplateApp.hpp
    WickedStyleTemplate/main.cpp
)
target_link_libraries(WickedStyleTemplate PRIVATE GameEngine imgui)
target_include_directories(WickedStyleTemplate PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})
target_compile_definitions(WickedStyleTemplate PRIVATE GAMEENGINE_INCLUDE_PATH="${CMAKE_SOURCE_DIR}/Engine")

# Example 12: Visual Scripting Demo
add_executable(VisualScriptingDemo 12_VisualScripting/VisualScriptingDemo.cpp)
target_link_libraries(VisualScriptingDemo PRIVATE GameEngine imgui)
target_include_directories(VisualScriptingDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

set(VULKAN_EXAMPLE_TARGETS
    PhysicsDemo
    CharacterController
    RobotArm
    WickedStyleTemplate
    VisualScriptingDemo
)

# ─── Legacy GL examples (gated) ────────────────────────────────────────────

if(ENABLE_LEGACY_GL_EXAMPLES)
    add_executable(HelloTriangle 01_HelloTriangle/HelloTriangle.cpp)
    target_link_libraries(HelloTriangle PRIVATE GameEngine imgui)
    target_include_directories(HelloTriangle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})
    target_compile_definitions(HelloTriangle PRIVATE GAMEENGINE_INCLUDE_PATH="${CMAKE_SOURCE_DIR}/Engine")

    add_executable(ClothSimulation 03_ClothSimulation/ClothSimulationApp.cpp)
    target_link_libraries(ClothSimulation PRIVATE GameEngine imgui)
    target_include_directories(ClothSimulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

    add_executable(FluidSimulation 05_FluidSimulation/FluidSimApp.cpp)
    target_link_libraries(FluidSimulation PRIVATE GameEngine imgui)
    target_include_directories(FluidSimulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

    add_executable(Physics2DDemo 07_Physics2DDemo/Physics2DDemoApp.cpp)
    target_link_libraries(Physics2DDemo PRIVATE GameEngine imgui)
    target_include_directories(Physics2DDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

    add_executable(HXPBDPhysics 08_HXPBDPhysics/HXPBDDemoApp.cpp)
    target_link_libraries(HXPBDPhysics PRIVATE GameEngine imgui)
    target_include_directories(HXPBDPhysics PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

    add_executable(FlagSimulation 09_FlagSimulation/FlagSimulationApp.cpp)
    target_link_libraries(FlagSimulation PRIVATE GameEngine imgui)
    target_include_directories(FlagSimulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

    add_executable(LegacyParticles 10_LegacyParticles/LegacyParticleDemoApp.cpp)
    target_link_libraries(LegacyParticles PRIVATE GameEngine imgui)
    target_include_directories(LegacyParticles PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

    add_executable(TerrainDemo 11_TerrainDemo/TerrainDemo.cpp)
    target_link_libraries(TerrainDemo PRIVATE GameEngine imgui)
    target_include_directories(TerrainDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${ENGINE_INCLUDE_DIR})

    list(APPEND VULKAN_EXAMPLE_TARGETS
        HelloTriangle ClothSimulation FluidSimulation
        Physics2DDemo HXPBDPhysics FlagSimulation LegacyParticles TerrainDemo)
endif()

# ─── Output paths + install ────────────────────────────────────────────────

set_target_properties(${VULKAN_EXAMPLE_TARGETS} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

install(TARGETS ${VULKAN_EXAMPLE_TARGETS}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
