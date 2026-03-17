cmake_minimum_required(VERSION 3.16)

project(SPHFluid VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(src include)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_executable(sph_fluid ${SOURCES})
target_compile_features(sph_fluid PRIVATE cxx_std_17)

find_package(SFML 3.0 COMPONENTS Graphics Window System REQUIRED)
target_link_libraries(sph_fluid PRIVATE SFML::Graphics SFML::Window SFML::System)

set_target_properties(sph_fluid PROPERTIES OUTPUT_NAME sph_fluid)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(sph_fluid PROPERTIES COMPILE_FLAGS "-O2")
endif()

install(TARGETS sph_fluid DESTINATION bin)
