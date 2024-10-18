add_executable(gpt4all_tests
    test_main.cpp
    basic_test.cpp
)

target_link_libraries(gpt4all_tests PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(gpt4all_tests)

# The 'check' target makes sure the tests are up-to-date before running them
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS gpt4all_tests)
