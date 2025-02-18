cmake_minimum_required(VERSION 3.29)
project(gpt4all-backend-test VERSION 0.1 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
include(../common/common.cmake)

add_subdirectory(../gpt4all-backend "${CMAKE_CURRENT_BINARY_DIR}/gpt4all-backend")
add_subdirectory(src)
