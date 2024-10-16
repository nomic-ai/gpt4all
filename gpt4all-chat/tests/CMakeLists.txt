add_executable(gpt4all_tests
    test_main.cpp
    basic_test.cpp
)

target_link_libraries(gpt4all_tests PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(gpt4all_tests)
