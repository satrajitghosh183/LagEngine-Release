cmake_minimum_required(VERSION 3.16)
project(ParticleSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_executable(particle_system Source.cpp)

target_include_directories(particle_system PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
)

target_link_libraries(particle_system
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
)

if(NOT MSVC)
    target_compile_options(particle_system PRIVATE -Wall -Wextra)
endif()
