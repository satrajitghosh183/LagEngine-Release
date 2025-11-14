# Examples CMakeLists.txt

# Example 1: Hello Triangle
add_executable(HelloTriangle
    01_HelloTriangle/HelloTriangle.cpp
)
target_link_libraries(HelloTriangle PRIVATE GameEngine)
target_include_directories(HelloTriangle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Engine)

# Example 2: Physics Demo
add_executable(PhysicsDemo
    02_PhysicsDemo/PhysicsDemoApp.cpp
)
target_link_libraries(PhysicsDemo PRIVATE GameEngine)
target_include_directories(PhysicsDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Engine)

# Example 3: Cloth Simulation
add_executable(ClothSimulation
    03_ClothSimulation/ClothSimulationApp.cpp
)
target_link_libraries(ClothSimulation PRIVATE GameEngine)
target_include_directories(ClothSimulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Engine)

# Example 4: Character Controller
add_executable(CharacterController
    04_CharacterController/CharacterControllerApp.cpp
)
target_link_libraries(CharacterController PRIVATE GameEngine)
target_include_directories(CharacterController PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Engine)

# Example 5: Fluid Simulation
add_executable(FluidSimulation
    05_FluidSimulation/FluidSimApp.cpp
)
target_link_libraries(FluidSimulation PRIVATE GameEngine)
target_include_directories(FluidSimulation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Engine)

# Example 6: Robot Arm
add_executable(RobotArm
    06_RobotArm/RobotArmApp.cpp
)
target_link_libraries(RobotArm PRIVATE GameEngine)
target_include_directories(RobotArm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Engine)

# Set output directory
set_target_properties(
    HelloTriangle
    PhysicsDemo
    ClothSimulation
    CharacterController
    FluidSimulation
    RobotArm
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
