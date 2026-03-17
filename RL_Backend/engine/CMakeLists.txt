# RL_Backend Engine - minimal C++20 engine for multithreaded rendering + RL

set(ENGINE_SOURCES
    src/JobSystem.cpp
    src/RenderThread.cpp
    src/IndirectDrawBuilder.cpp
    src/Window.cpp
    src/Shader.cpp
    src/NBodySim.cpp
)

set(ENGINE_HEADERS
    include/rldemo/JobSystem.hpp
    include/rldemo/RenderThread.hpp
    include/rldemo/IndirectDrawBuilder.hpp
    include/rldemo/Window.hpp
    include/rldemo/Shader.hpp
    include/rldemo/Types.hpp
    include/rldemo/NBodySim.hpp
    include/rldemo/CudaPhysics.hpp
    include/rldemo/nbody_cuda.cuh
)

if(RL_BACKEND_BUILD_CUDA)
    list(APPEND ENGINE_SOURCES
        src/cuda/cuda_physics.cu
        src/cuda/nbody_gravity.cu
        src/cuda/collision_broadphase.cu
        src/cuda/integrate.cu
    )
else()
    list(APPEND ENGINE_SOURCES src/CudaPhysics_stub.cpp)
endif()

add_library(rldemo_engine ${ENGINE_SOURCES} ${ENGINE_HEADERS})

target_include_directories(rldemo_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(rldemo_engine
    PUBLIC
        glfw
        glad
        glm
        imgui
        spdlog::spdlog
)

if(WIN32)
    target_link_libraries(rldemo_engine PRIVATE opengl32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(rldemo_engine PRIVATE GL X11 pthread dl)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OPENGL_LIBRARY OpenGL REQUIRED)
    target_link_libraries(rldemo_engine PRIVATE
        ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY} ${OPENGL_LIBRARY})
endif()

set_target_properties(rldemo_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(rldemo_engine PRIVATE GLFW_INCLUDE_NONE)
target_compile_features(rldemo_engine PUBLIC cxx_std_20)

if(RL_BACKEND_BUILD_CUDA)
    set_target_properties(rldemo_engine PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
        CUDA_ARCHITECTURES "75;86;89"
    )
    target_link_libraries(rldemo_engine PRIVATE CUDA::cudart)
    target_compile_options(rldemo_engine PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcudafe --diag_suppress=esa_on_defaulted_function_ignored>)
endif()
