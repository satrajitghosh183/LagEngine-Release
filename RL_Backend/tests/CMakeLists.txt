add_executable(test_jobs test_jobs.cpp)
target_link_libraries(test_jobs PRIVATE rldemo_engine Catch2::Catch2WithMain)
target_include_directories(test_jobs PRIVATE ${CMAKE_SOURCE_DIR}/engine/include)

include(CTest)
add_test(NAME JobSystemTests COMMAND test_jobs)
