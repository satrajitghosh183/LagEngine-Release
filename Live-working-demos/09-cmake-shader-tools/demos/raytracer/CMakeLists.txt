cmake_minimum_required(VERSION 3.16)

project(MiniRay VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(src include)

find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

add_executable(miniray src/main.cpp)

target_link_libraries(miniray PRIVATE SFML::Graphics SFML::Window SFML::System)

set_target_properties(miniray PROPERTIES OUTPUT_NAME miniray)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_property(TARGET miniray APPEND_STRING PROPERTY COMPILE_FLAGS " -O2")
endif()

install(TARGETS miniray DESTINATION bin)
