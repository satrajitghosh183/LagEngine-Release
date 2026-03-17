add_executable(demo_hello_triangle main.cpp)

target_link_libraries(demo_hello_triangle
    PRIVATE
        rldemo_engine
)

target_include_directories(demo_hello_triangle PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)
