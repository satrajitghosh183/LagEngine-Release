cmake_minimum_required(VERSION 3.21)
project(XPBDSoftBody LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

# Add the shared VulkanBase library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common ${CMAKE_CURRENT_BINARY_DIR}/common)

include_directories(core/materials core core/utils core/meshes core/lights core/cameras ${CMAKE_SOURCE_DIR})
file(GLOB SRCS ./*.h ./*.cpp
        ./core/*.cpp ./core/*.h
        ./core/cameras/*.cpp ./core/cameras/*.h
        ./core/lights/*.cpp ./core/lights/*.h
        core/meshes/*.cpp core/meshes/*.h
        core/materials/*.cpp core/materials/*.h
        core/utils/*.cpp core/utils/*.h
        core/physics/*.cpp core/physics/*.h
        core/shadows/*.cpp core/shadows/*.h
)

add_executable(XPBDSoftBody ${SRCS}
        core/physics/UniformAccelerationField.h
        core/physics/DihedralBendConstraint.h
        core/physics/ConstraintHelper.h
        core/Renderable.h
        core/AABB.h
        core/AABBHelper.h
        core/physics/GlobalVolumeConstraint.h
        core/PickResult.h
        core/physics/PhysicsBodyHelper.h
)

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})

target_link_libraries(${PROJECT_NAME} VulkanBase)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

# Eigen
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Eigen)

# shaderc for runtime GLSL->SPIR-V compilation
find_package(Vulkan REQUIRED)
if(TARGET Vulkan::shaderc_combined)
    target_link_libraries(${PROJECT_NAME} Vulkan::shaderc_combined)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_SHADERC=1)
elseif(EXISTS "${Vulkan_INCLUDE_DIR}/../Lib/shaderc_combined.lib")
    target_link_libraries(${PROJECT_NAME} "${Vulkan_INCLUDE_DIR}/../Lib/shaderc_combined.lib")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_SHADERC=1)
else()
    # Try pkg-config as fallback
    find_library(SHADERC_LIB shaderc_combined)
    if(SHADERC_LIB)
        target_link_libraries(${PROJECT_NAME} ${SHADERC_LIB})
        target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_SHADERC=1)
    else()
        message(WARNING "shaderc not found - runtime shader compilation disabled, using embedded SPIR-V only")
    endif()
endif()

# Optimization
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /O2)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3)
endif()
