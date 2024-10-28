include(FetchContent)

find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

# Google test download and setup
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

configure_file(python/config.py.in "${CMAKE_CURRENT_SOURCE_DIR}/python/config.py")

add_test(NAME ChatPythonTests
    COMMAND ${Python3_EXECUTABLE} -m pytest --color=yes "${CMAKE_CURRENT_SOURCE_DIR}/python"
)
set_tests_properties(ChatPythonTests PROPERTIES
    ENVIRONMENT "CHAT_EXECUTABLE=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/chat;TEST_MODEL_PATH=${TEST_MODEL_PATH}"
    TIMEOUT 60
)

add_executable(gpt4all_tests
    cpp/test_main.cpp
    cpp/basic_test.cpp
)

target_link_libraries(gpt4all_tests PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(gpt4all_tests)
