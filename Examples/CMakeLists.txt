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

# Set output directory
set_target_properties(
    HelloTriangle
    PhysicsDemo
    ClothSimulation
    CharacterController
    FluidSimulation
    RobotArm
    WickedStyleTemplate
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
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
