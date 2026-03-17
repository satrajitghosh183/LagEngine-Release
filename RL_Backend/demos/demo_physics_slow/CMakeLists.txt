add_executable(demo_physics_slow main.cpp)

target_link_libraries(demo_physics_slow
    PRIVATE
        rldemo_engine
)

target_include_directories(demo_physics_slow PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)
