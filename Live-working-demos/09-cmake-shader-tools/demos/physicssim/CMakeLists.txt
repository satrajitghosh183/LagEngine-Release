cmake_minimum_required(VERSION 3.16)

project(PhysicsSim VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
target_compile_features(physics_sim PRIVATE cxx_std_17)
add_compile_options(-Wall -Wextra -Wpedantic)

# Include directories
include_directories(include)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Header files
file(GLOB_RECURSE HEADERS "include/*.h")

# SFML find package
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Add executable target
add_executable(physics_sim ${SOURCES} ${HEADERS})

# Link SFML libraries
target_link_libraries(physics_sim PRIVATE sfml-graphics sfml-window sfml-system)

# Define compile definition for Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(PHYSICS_DEBUG)
endif()

# Install rules if target is a library (not needed here as it's an executable)
# install(TARGETS physics_sim DESTINATION bin)

# Output binary name
set_target_properties(physics_sim PROPERTIES OUTPUT_NAME "physics_sim")

# Additional configuration for different build types can be added here
