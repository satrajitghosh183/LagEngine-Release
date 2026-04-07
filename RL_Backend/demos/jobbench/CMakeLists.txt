add_executable(jobbench main.cpp)

target_link_libraries(jobbench
    PRIVATE
        rldemo_engine
)

target_include_directories(jobbench PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)
