cmake_minimum_required(VERSION 3.20)

# Shared Vulkan bootstrap library for Live-working-demos
add_library(VulkanBase STATIC
    VulkanBase.cpp
    VulkanBase.hpp
)

target_include_directories(VulkanBase PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(VulkanBase PUBLIC Vulkan::Vulkan)

# GLFW (from parent External/ if available, otherwise find system)
if(TARGET glfw)
    target_link_libraries(VulkanBase PUBLIC glfw)
else()
    find_package(glfw3 3.3 REQUIRED)
    target_link_libraries(VulkanBase PUBLIC glfw)
endif()

# ImGui (expected to be provided by parent project or found locally)
if(TARGET imgui)
    target_link_libraries(VulkanBase PUBLIC imgui)
else()
    # Fallback: compile ImGui sources inline
    set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../External/imgui")
    if(EXISTS "${IMGUI_DIR}/imgui.cpp")
        add_library(imgui_demo STATIC
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            ${IMGUI_DIR}/imgui_demo.cpp
            ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
            ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        )
        target_include_directories(imgui_demo PUBLIC
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
        )
        target_link_libraries(imgui_demo PUBLIC Vulkan::Vulkan glfw)
        target_link_libraries(VulkanBase PUBLIC imgui_demo)
    endif()
endif()

# GLM
if(TARGET glm)
    target_link_libraries(VulkanBase PUBLIC glm)
else()
    set(GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../External/glm")
    if(EXISTS "${GLM_DIR}/glm/glm.hpp")
        target_include_directories(VulkanBase PUBLIC ${GLM_DIR})
    else()
        find_package(glm REQUIRED)
        target_link_libraries(VulkanBase PUBLIC glm::glm)
    endif()
endif()

target_compile_features(VulkanBase PUBLIC cxx_std_17)
