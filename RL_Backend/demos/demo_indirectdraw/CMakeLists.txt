add_executable(demo_indirectdraw main.cpp)

target_link_libraries(demo_indirectdraw
    PRIVATE
        rldemo_engine
)

target_include_directories(demo_indirectdraw PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)
