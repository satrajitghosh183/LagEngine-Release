cmake_minimum_required(VERSION 3.20)
project(VerletPhysics LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Shared Vulkan bootstrap library ===
add_subdirectory(../common common_build)

# === Vulkan SDK (for shaderc runtime compile) ===
find_package(Vulkan REQUIRED)

# === Directory Paths ===
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(ENGINE_DIR ${CMAKE_SOURCE_DIR}/engine)

# === Engine Core Shared Sources ===
file(GLOB_RECURSE ENGINE_CORE_SOURCES
    ${ENGINE_DIR}/core/*.cpp
)

# === 2D Module Sources ===
set(ENGINE_2D_SOURCES
    ${ENGINE_DIR}/objects/Ball2D.cpp
    ${ENGINE_DIR}/objects/Cloth2D.cpp
    ${ENGINE_DIR}/objects/Cannon2D.cpp
    ${ENGINE_DIR}/physics/Particle.cpp
    ${ENGINE_DIR}/physics/Constraint2D.cpp
    ${ENGINE_DIR}/scene/Scene2D.cpp
    ${ENGINE_DIR}/scene/SceneManager2D.cpp
)

# === 3D Module Sources (physics + data only) ===
set(ENGINE_3D_SOURCES
    ${ENGINE_DIR}/graphics/Camera.cpp
    ${ENGINE_DIR}/graphics/MeshGenerator3D.cpp
    ${ENGINE_DIR}/objects/Ball3D.cpp
    ${ENGINE_DIR}/physics/Particle3D.cpp
    ${ENGINE_DIR}/physics/Constraint3D.cpp
    ${ENGINE_DIR}/physics/Spring3D.cpp
    ${ENGINE_DIR}/physics/VertletSystem3D.cpp
    ${ENGINE_DIR}/physics/ClothSolver3D.cpp
    ${ENGINE_DIR}/physics/PhysicsWorld3D.cpp
    ${ENGINE_DIR}/scene/Scene3D.cpp
    ${ENGINE_DIR}/scene/SceneManager3D.cpp
)

# === Try to find shaderc (comes with Vulkan SDK) ===
find_library(SHADERC_LIB
    NAMES shaderc_combined shaderc_shared
    HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/Lib"
)

# === 2D Executable (Verlet cloth + cannon, Vulkan) ===
add_executable(verlet_2d
    ${SRC_DIR}/main2D.cpp
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_2D_SOURCES}
)

target_include_directories(verlet_2d PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(verlet_2d
    VulkanBase
)

if(SHADERC_LIB)
    target_link_libraries(verlet_2d ${SHADERC_LIB})
    target_compile_definitions(verlet_2d PRIVATE HAS_SHADERC=1)
else()
    message(STATUS "shaderc not found -- verlet_2d requires pre-compiled SPIR-V")
endif()

# === 3D Executable (Verlet cloth + balls, Vulkan) ===
add_executable(verlet_3d
    ${SRC_DIR}/main3D.cpp
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_3D_SOURCES}
)

target_include_directories(verlet_3d PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(verlet_3d
    VulkanBase
)

if(SHADERC_LIB)
    target_link_libraries(verlet_3d ${SHADERC_LIB})
    target_compile_definitions(verlet_3d PRIVATE HAS_SHADERC=1)
else()
    message(STATUS "shaderc not found -- verlet_3d requires pre-compiled SPIR-V")
endif()

# === SPH Water Executable (Vulkan) ===
add_executable(sph_water
    ${SRC_DIR}/sph_water.cpp
)

target_include_directories(sph_water PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(sph_water
    VulkanBase
)

if(SHADERC_LIB)
    target_link_libraries(sph_water ${SHADERC_LIB})
    target_compile_definitions(sph_water PRIVATE HAS_SHADERC=1)
else()
    message(STATUS "shaderc not found -- sph_water requires pre-compiled SPIR-V")
endif()

# === Rigid Body Test (console-only, no rendering) ===
# Collect all physics source files needed for the rigid body test
file(GLOB PHYSICS_COLLISION_SOURCES
    ${ENGINE_DIR}/physics/collision/*.cpp
)
file(GLOB PHYSICS_CONSTRAINTS_SOURCES
    ${ENGINE_DIR}/physics/constraints/*.cpp
)
file(GLOB PHYSICS_SHAPES_SOURCES
    ${ENGINE_DIR}/physics/shapes/*.cpp
)

set(ENGINE_RIGIDBODY_SOURCES
    ${ENGINE_DIR}/physics/RigidBody.cpp
    ${ENGINE_DIR}/physics/PhysicsWorld.cpp
    ${ENGINE_DIR}/physics/CollisonShape.cpp
    ${ENGINE_DIR}/physics/BoxShape.cpp
    ${ENGINE_DIR}/physics/SphereShape.cpp
    ${ENGINE_DIR}/physics/SoftBodySystem.cpp
    ${ENGINE_DIR}/physics/SoftBodySystem3d.cpp
    ${PHYSICS_COLLISION_SOURCES}
    ${PHYSICS_CONSTRAINTS_SOURCES}
    ${PHYSICS_SHAPES_SOURCES}
)

add_executable(test_rigid_bodies
    ${SRC_DIR}/testRigidBodies.cpp
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_RIGIDBODY_SOURCES}
)

target_include_directories(test_rigid_bodies PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# GLM (needed for physics math)
# VulkanBase already provides GLM include paths
target_link_libraries(test_rigid_bodies
    VulkanBase
)

if(NOT MSVC)
    target_compile_options(verlet_2d PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_options(verlet_3d PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_options(sph_water PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_compile_options(test_rigid_bodies PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
