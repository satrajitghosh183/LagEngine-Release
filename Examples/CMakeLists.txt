# Examples CMakeLists.txt

# Create an interface target that maps Engine -> GameEngine includes
# This allows examples to use #include <GameEngine/...> style includes
set(ENGINE_INCLUDE_DIR ${CMAKE_SOURCE_DIR})

# Example 1: Hello Triangle
add_executable(HelloTriangle
    01_HelloTriangle/HelloTriangle.cpp
)
target_link_libraries(HelloTriangle PRIVATE GameEngine imgui)
target_include_directories(HelloTriangle PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${ENGINE_INCLUDE_DIR}
)
target_compile_definitions(HelloTriangle PRIVATE GAMEENGINE_INCLUDE_PATH="${CMAKE_SOURCE_DIR}/Engine")

# Example 2: Physics Demo
add_executable(PhysicsDemo
    02_PhysicsDemo/PhysicsDemoApp.cpp
)
target_link_libraries(PhysicsDemo PRIVATE GameEngine imgui)
target_include_directories(PhysicsDemo PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${ENGINE_INCLUDE_DIR}
)

# Example 3: Cloth Simulation
add_executable(ClothSimulation
    03_ClothSimulation/ClothSimulationApp.cpp
)
target_link_libraries(ClothSimulation PRIVATE GameEngine imgui)
target_include_directories(ClothSimulation PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${ENGINE_INCLUDE_DIR}
)

# Example 4: Character Controller
add_executable(CharacterController
    04_CharacterController/CharacterControllerApp.cpp
)
target_link_libraries(CharacterController PRIVATE GameEngine imgui)
target_include_directories(CharacterController PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${ENGINE_INCLUDE_DIR}
)

# Example 5: Fluid Simulation
add_executable(FluidSimulation
    05_FluidSimulation/FluidSimApp.cpp
)
target_link_libraries(FluidSimulation PRIVATE GameEngine imgui)
target_include_directories(FluidSimulation PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${ENGINE_INCLUDE_DIR}
)

# Example 6: Robot Arm
add_executable(RobotArm
    06_RobotArm/RobotArmApp.cpp
)
target_link_libraries(RobotArm PRIVATE GameEngine imgui)
target_include_directories(RobotArm PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${ENGINE_INCLUDE_DIR}
)

# Example 7: Wicked Style Template
add_executable(WickedStyleTemplate
    WickedStyleTemplate/WickedStyleTemplateApp.cpp
    WickedStyleTemplate/WickedStyleTemplateApp.hpp
    WickedStyleTemplate/main.cpp
)
target_link_libraries(WickedStyleTemplate PRIVATE GameEngine imgui)
target_include_directories(WickedStyleTemplate PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${ENGINE_INCLUDE_DIR}
)
target_compile_definitions(WickedStyleTemplate PRIVATE GAMEENGINE_INCLUDE_PATH="${CMAKE_SOURCE_DIR}/Engine")

# -----------------------------------------------------------------------
# Examples from old_code/ migration (Option B — physics 1:1, thin wrapper)
# -----------------------------------------------------------------------

# Example 07: 2D Physics Demo  (old_code/src/main2D.cpp)
# Cloth2D + Cannon2D + Ball2D with Verlet integration — sf::Vector2f → glm::vec2
add_executable(Physics2DDemo
    07_Physics2DDemo/Physics2DDemoApp.cpp
)
target_link_libraries(Physics2DDemo PRIVATE GameEngine imgui)
target_include_directories(Physics2DDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}
)

# Example 08: HXPBD Cloth Physics  (old_code/feather/)
# XPBD velocity/constraint/position update loop 1:1 from HXPbdSolver::solve()
add_executable(HXPBDPhysics
    08_HXPBDPhysics/HXPBDDemoApp.cpp
)
target_link_libraries(HXPBDPhysics PRIVATE GameEngine imgui)
target_include_directories(HXPBDPhysics PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}
)

# Example 09: Flag Simulation  (old_code/cloth-simulation/src/flag.cpp)
# Spring-mass cloth: hookForce / brakeForce / repulseForce / Flag::update() 1:1
add_executable(FlagSimulation
    09_FlagSimulation/FlagSimulationApp.cpp
)
target_link_libraries(FlagSimulation PRIVATE GameEngine imgui)
target_include_directories(FlagSimulation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}
)

# Example 10: Legacy Particle System  (old_code/particle-sim/)
# Gravity + friction + floor-bounce + inter-particle collision 1:1 from Source.cpp
add_executable(LegacyParticles
    10_LegacyParticles/LegacyParticleDemoApp.cpp
)
target_link_libraries(LegacyParticles PRIVATE GameEngine imgui)
target_include_directories(LegacyParticles PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}
)

# Example 11: Terrain Demo
add_executable(TerrainDemo
    11_TerrainDemo/TerrainDemo.cpp
)
target_link_libraries(TerrainDemo PRIVATE GameEngine imgui)
target_include_directories(TerrainDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}
)

# Example 12: Visual Scripting Demo
add_executable(VisualScriptingDemo
    12_VisualScripting/VisualScriptingDemo.cpp
)
target_link_libraries(VisualScriptingDemo PRIVATE GameEngine imgui)
target_include_directories(VisualScriptingDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ENGINE_INCLUDE_DIR}
)

# Set output directory
set_target_properties(
    HelloTriangle
    PhysicsDemo
    ClothSimulation
    CharacterController
    FluidSimulation
    RobotArm
    WickedStyleTemplate
    Physics2DDemo
    HXPBDPhysics
    FlagSimulation
    LegacyParticles
    TerrainDemo
    VisualScriptingDemo
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install example executables
install(TARGETS
    HelloTriangle
    PhysicsDemo
    ClothSimulation
    CharacterController
    FluidSimulation
    RobotArm
    WickedStyleTemplate
    Physics2DDemo
    HXPBDPhysics
    FlagSimulation
    LegacyParticles
    TerrainDemo
    VisualScriptingDemo
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
