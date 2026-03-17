add_executable(demo_physics_rl main.cpp)

target_link_libraries(demo_physics_rl
    PRIVATE
        rldemo_engine
)

target_include_directories(demo_physics_rl PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)
