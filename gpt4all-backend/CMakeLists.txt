cmake_minimum_required(VERSION 3.16)

if(APPLE)
  option(BUILD_UNIVERSAL "Build a Universal binary on macOS" ON)
  if(BUILD_UNIVERSAL)
    # Build a Universal binary on macOS
    # This requires that the found Qt library is compiled as Universal binaries.
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  else()
    # Build for the host architecture on macOS
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
  endif()
endif()

# Include the binary directory for the generated header file
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(LLMODEL_VERSION_MAJOR 0)
set(LLMODEL_VERSION_MINOR 1)
set(LLMODEL_VERSION_PATCH 1)
set(LLMODEL_VERSION "${LLMODEL_VERSION_MAJOR}.${LLMODEL_VERSION_MINOR}.${LLMODEL_VERSION_PATCH}")
project(llmodel VERSION ${LLMODEL_VERSION} LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LLAMA_BUILD_EXAMPLES ON CACHE BOOL "llama: build examples" FORCE)
set(BUILD_SHARED_LIBS ON FORCE)

set(CMAKE_VERBOSE_MAKEFILE ON)
if (GPT4ALL_AVX_ONLY)
    set(LLAMA_AVX2 OFF CACHE BOOL "llama: enable AVX2" FORCE)
    set(LLAMA_F16C OFF CACHE BOOL "llama: enable F16C" FORCE)
    set(LLAMA_FMA  OFF CACHE BOOL "llama: enable FMA" FORCE)
endif()

add_subdirectory(llama.cpp)

add_library(llmodel
    gptj.h gptj.cpp
    llamamodel.h llamamodel.cpp
    llama.cpp/examples/common.cpp
    llmodel.h llmodel_c.h llmodel_c.cpp
    mpt.h mpt.cpp
    utils.h utils.cpp
)

target_link_libraries(llmodel
    PRIVATE llama)

set_target_properties(llmodel PROPERTIES
                              VERSION ${PROJECT_VERSION}
                              SOVERSION ${PROJECT_VERSION_MAJOR})

set(COMPONENT_NAME_MAIN ${PROJECT_NAME})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
