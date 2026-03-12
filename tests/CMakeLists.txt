find_package(Qt6 6.2 REQUIRED COMPONENTS Test Sql)

# Each test file has its own QTEST_GUILESS_MAIN — they must be separate executables.

add_executable(test_engine test_engine.cpp)
target_include_directories(test_engine PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_engine PRIVATE dupecheck_engine Qt6::Test)
add_test(NAME test_engine COMMAND test_engine)

add_executable(test_repository test_repository.cpp)
target_include_directories(test_repository PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_repository PRIVATE dupecheck_engine Qt6::Test Qt6::Sql)
add_test(NAME test_repository COMMAND test_repository)
