cmake_minimum_required(VERSION 3.16)

project(HelloGraphics VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)
if(SFML_FOUND)
    add_executable(hello_sfml ${SOURCES})
    target_compile_features(hello_sfml PRIVATE cxx_std_17)
    target_link_libraries(hello_sfml SFML::Graphics SFML::Window SFML::System)
else()
    message(FATAL_ERROR "SFML not found, please install it.")
endif()

add_compile_options(-O2)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options($<$<CONFIG:Release>:-O3>)
endif()

install(TARGETS hello_sfml DESTINATION bin)
