include(FetchContent)

# Google test download and setup
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_test(NAME ChatPythonTests
    COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_SOURCE_DIR}/tests/python_tests
)
set_tests_properties(ChatPythonTests PROPERTIES
    ENVIRONMENT "CHAT_EXECUTABLE=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/chat"
    TIMEOUT 60
)

add_executable(gpt4all_tests
    test_main.cpp
    basic_test.cpp
)

target_link_libraries(gpt4all_tests PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(gpt4all_tests)
