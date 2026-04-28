cmake_minimum_required(VERSION 3.20)
project(ParticleSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../common common_build)

add_executable(particle_system Source.cpp)

target_include_directories(particle_system PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(particle_system
    VulkanBase
)

# Link shaderc for runtime GLSL->SPIR-V compilation.
# shaderc_combined is bundled with the Vulkan SDK.
find_package(Vulkan REQUIRED)

find_library(SHADERC_LIB
    NAMES shaderc_combined shaderc_shared
    HINTS "$ENV{VULKAN_SDK}/lib" "$ENV{VULKAN_SDK}/Lib"
          "${Vulkan_INCLUDE_DIR}/../lib"
)
if(SHADERC_LIB)
    target_link_libraries(particle_system ${SHADERC_LIB})
    target_compile_definitions(particle_system PRIVATE HAS_SHADERC=1)
    message(STATUS "Found shaderc: ${SHADERC_LIB}")
else()
    message(WARNING "shaderc not found -- particle_system will fail at runtime "
                    "unless you provide pre-compiled SPIR-V. Install the Vulkan SDK.")
endif()

# shaderc include directory (typically alongside Vulkan headers)
if(Vulkan_INCLUDE_DIR)
    target_include_directories(particle_system PRIVATE ${Vulkan_INCLUDE_DIR})
endif()

if(NOT MSVC)
    target_compile_options(particle_system PRIVATE -Wall -Wextra)
endif()
