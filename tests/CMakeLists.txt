find_package(Qt6 6.2 REQUIRED COMPONENTS Test Sql)

add_executable(dupecheck_tests
    test_engine.cpp
    test_repository.cpp
)

target_include_directories(dupecheck_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(dupecheck_tests
    PRIVATE
    dupecheck_engine
    Qt6::Test
    Qt6::Sql
)

qt_add_resources(dupecheck_tests "test_resources" FILES)

add_test(NAME dupecheck_tests COMMAND dupecheck_tests)
