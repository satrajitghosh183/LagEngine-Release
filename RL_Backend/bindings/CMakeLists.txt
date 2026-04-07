# pybind11 module for RL environments
# Exposes FrameSchedulerEnv and WorkStealSchedulerEnv

if(NOT RL_BACKEND_BUILD_RL)
    return()
endif()

if(NOT TARGET pybind11::pybind11)
    message(WARNING "pybind11 not found - skipping RL bindings")
    return()
endif()

pybind11_add_module(rldemo_env bindings.cpp)

target_link_libraries(rldemo_env PRIVATE rldemo_engine pybind11::pybind11)
target_include_directories(rldemo_env PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
)

# Output to both rl/ (for direct import) and lib/ (standard location)
set_target_properties(rldemo_env PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Also copy to rl directory for easy Python import
add_custom_command(TARGET rldemo_env POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:rldemo_env>
        ${CMAKE_SOURCE_DIR}/rl/
    COMMENT "Copying rldemo_env to rl/ for Python import"
)
