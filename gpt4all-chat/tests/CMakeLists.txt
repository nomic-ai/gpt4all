include(FetchContent)

find_package(Python3 3.12 REQUIRED COMPONENTS Interpreter)

# Google test download and setup
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

# Llama-3.2-1B model
set(TEST_MODEL "Llama-3.2-1B-Instruct-Q4_0.gguf")
set(TEST_MODEL_MD5 "48ff0243978606fdba19d899b77802fc")
set(TEST_MODEL_PATH "${CMAKE_BINARY_DIR}/resources/${TEST_MODEL}")
set(TEST_MODEL_URL "https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/resolve/main/${TEST_MODEL}")
message(STATUS "Downloading test model from ${TEST_MODEL_URL} ...")
file(DOWNLOAD
    "${TEST_MODEL_URL}"
    "${TEST_MODEL_PATH}"
    EXPECTED_HASH "MD5=${TEST_MODEL_MD5}"
)
message(STATUS "Test model downloaded to ${TEST_MODEL_PATH}")

configure_file(python/config.py.in "${CMAKE_CURRENT_SOURCE_DIR}/python/config.py")

add_test(NAME ChatPythonTests
    COMMAND ${Python3_EXECUTABLE} -m pytest -s --color=yes "${CMAKE_CURRENT_SOURCE_DIR}/python"
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
