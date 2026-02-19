cmake_minimum_required(VERSION 3.16)
project(CustomGameEngine)

set(CMAKE_CXX_STANDARD 17)

# === Directory Paths ===
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(ENGINE_DIR ${CMAKE_SOURCE_DIR}/engine)
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/external)
set(GLAD_DIR ${EXTERNAL_DIR}/glad)

# === Include Directories ===
include_directories(${ENGINE_DIR})
include_directories(${CMAKE_SOURCE_DIR})      # src/
include_directories(${EXTERNAL_DIR})
include_directories(${GLAD_DIR}/include)

# === External Dependencies ===
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(SFML 2.5 COMPONENTS graphics window system system REQUIRED)

# === GLAD ===
file(GLOB GLAD_SOURCES ${GLAD_DIR}/src/*.c)

# === Engine Core Shared Sources ===
file(GLOB_RECURSE ENGINE_CORE_SOURCES
    ${ENGINE_DIR}/core/*.cpp
)

# === 2D Module Sources ===
file(GLOB_RECURSE ENGINE_2D_SOURCES
    ${ENGINE_DIR}/objects/Ball2D.cpp
    ${ENGINE_DIR}/objects/Cloth2D.cpp
    ${ENGINE_DIR}/objects/Cannon2D.cpp
    ${ENGINE_DIR}/physics/Particle.cpp
    ${ENGINE_DIR}/physics/Constraint2D.cpp
    ${ENGINE_DIR}/scene/Scene2D.cpp
    ${ENGINE_DIR}/scene/SceneManager2D.cpp
)

# === 3D Module Sources ===
# file(GLOB_RECURSE ENGINE_3D_SOURCES
#     ${ENGINE_DIR}/graphics/*.cpp
#     ${ENGINE_DIR}/objects/Ball3D.cpp
#     ${ENGINE_DIR}/objects/Cloth3D.cpp
#     ${ENGINE_DIR}/physics/Particle3D.cpp
#     ${ENGINE_DIR}/physics/Constraint3D.cpp
#     ${ENGINE_DIR}/physics/PhysicsWorld3D.cpp
#     ${ENGINE_DIR}/scene/Scene3D.cpp
#     ${ENGINE_DIR}/scene/SceneManager3D.cpp
    
# )


# === 3D Module Sources ===
file(GLOB_RECURSE ENGINE_3D_SOURCES
    ${ENGINE_DIR}/graphics/Camera.cpp
    ${ENGINE_DIR}/graphics/Shader.cpp
    ${ENGINE_DIR}/graphics/Mesh.cpp         # <-- ADD THIS (was missing)
    ${ENGINE_DIR}/graphics/Mesh3D.cpp
    ${ENGINE_DIR}/graphics/MeshGenerator3D.cpp
    ${ENGINE_DIR}/graphics/Texture2D.cpp

    ${EXTERNAL_DIR}/stb_image.cpp


    ${ENGINE_DIR}/objects/Ball3D.cpp
    ${ENGINE_DIR}/objects/Cloth3D.cpp

    ${ENGINE_DIR}/physics/Particle3D.cpp
    ${ENGINE_DIR}/physics/Constraint3D.cpp  # <-- ADD THIS (was missing)
    ${ENGINE_DIR}/physics/Spring3D.cpp
    ${ENGINE_DIR}/physics/VertletSystem3D.cpp
    ${ENGINE_DIR}/physics/ClothSolver3D.cpp
    ${ENGINE_DIR}/physics/PhysicsWorld3D.cpp

    ${ENGINE_DIR}/scene/Scene3D.cpp
    ${ENGINE_DIR}/scene/SceneManager3D.cpp  # <-- ADD THIS (was missing)
)


# === 2D Executable ===
add_executable(main2D
    ${SRC_DIR}/main2D.cpp
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_2D_SOURCES}
)

target_link_libraries(main2D
    sfml-graphics
    sfml-window
    sfml-system
)

# === 3D Executable ===
add_executable(main3D
    ${SRC_DIR}/main3D.cpp
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_3D_SOURCES}
    ${GLAD_SOURCES}
)

target_link_libraries(main3D
    ${OPENGL_LIBRARIES}
    glfw
    sfml-system
)

