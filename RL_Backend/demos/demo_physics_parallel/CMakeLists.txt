add_executable(demo_physics_parallel main.cpp)

target_link_libraries(demo_physics_parallel
    PRIVATE
        rldemo_engine
)

target_include_directories(demo_physics_parallel PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)
