cmake_minimum_required(VERSION 3.29)

project(gpt4all-backend VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 6.8 COMPONENTS Core REQUIRED)

add_subdirectory(../deps "${CMAKE_CURRENT_BINARY_DIR}/common_deps")
add_subdirectory(deps)
add_subdirectory(src)
