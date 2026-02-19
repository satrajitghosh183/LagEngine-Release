cmake_minimum_required(VERSION 3.8)
project(bulletOpenGL)

set(CMAKE_CXX_STANDARD 11)

# config OpenGL
FIND_PACKAGE(OpenGL REQUIRED)
LIST(APPEND LIBS ${OPENGL_gl_LIBRARY})
LIST(APPEND LIBS ${OPENGL_glu_LIBRARY})
INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
message(warning ${OPENGL_INCLUDE_DIR})

# config freeglut
# TODO: NEED modify
add_subdirectory(3rd_party/freeglut/freeglut/freeglut)
INCLUDE_DIRECTORIES(3rd_party/freeglut/freeglut/freeglut/include)
#LIST(APPEND LIBS freeglut)
LIST(APPEND LIBS glut)

# config bullet
option(BUILD_BULLET2_DEMOS OFF)
option(BUILD_CPU_DEMOS OFF)
option(BUILD_EXTRAS OFF)
option(BUILD_OPENGL3_DEMOS OFF)
option(BUILD_UNIT_TESTS OFF)
add_subdirectory(3rd_party/bullet)
INCLUDE_DIRECTORIES(3rd_party/bullet/src)
LIST(APPEND LIBS BulletDynamics BulletCollision BulletCollision LinearMath)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -std=c++11")
endif()

add_subdirectory(demo1)
add_subdirectory(demo2)
add_subdirectory(demo3)
add_subdirectory(demo4)
add_subdirectory(demo5)
add_subdirectory(demo6)
add_subdirectory(demo7)
add_subdirectory(demo8)
add_subdirectory(demo9)
add_subdirectory(demo10)
add_subdirectory(demo11)
add_subdirectory(demo12)
add_subdirectory(demo13)
add_subdirectory(demo14)
add_subdirectory(demo15)
add_subdirectory(demo16)
add_subdirectory(demo17)
add_subdirectory(demo18)