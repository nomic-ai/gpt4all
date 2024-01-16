cmake_minimum_required(VERSION 3.16)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  option(BUILD_UNIVERSAL "Build a Universal binary on macOS" ON)
  if(BUILD_UNIVERSAL)
    # Build a Universal binary on macOS
    # This requires that the found Qt library is compiled as Universal binaries.
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  else()
    # Build for the host architecture on macOS
    if(NOT CMAKE_OSX_ARCHITECTURES)
      set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
    endif()
  endif()
endif()

# Include the binary directory for the generated header file
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(LLMODEL_VERSION_MAJOR 0)
set(LLMODEL_VERSION_MINOR 5)
set(LLMODEL_VERSION_PATCH 0)
set(LLMODEL_VERSION "${LLMODEL_VERSION_MAJOR}.${LLMODEL_VERSION_MINOR}.${LLMODEL_VERSION_PATCH}")
project(llmodel VERSION ${LLMODEL_VERSION} LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(BUILD_SHARED_LIBS ON)

# Check for IPO support
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if (NOT IPO_SUPPORTED)
    message(WARNING "Interprocedural optimization is not supported by your toolchain! This will lead to bigger file sizes and worse performance: ${IPO_ERROR}")
else()
    message(STATUS "Interprocedural optimization support detected")
endif()

if(NOT APPLE)
  set(LLAMA_KOMPUTE YES)
endif()

include(llama.cpp.cmake)

set(BUILD_VARIANTS default avxonly)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(BUILD_VARIANTS ${BUILD_VARIANTS} metal)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

# Go through each build variant
foreach(BUILD_VARIANT IN LISTS BUILD_VARIANTS)
    # Determine flags
    if (BUILD_VARIANT STREQUAL avxonly)
        set(GPT4ALL_ALLOW_NON_AVX NO)
    else()
        set(GPT4ALL_ALLOW_NON_AVX YES)
    endif()
    set(LLAMA_AVX2 ${GPT4ALL_ALLOW_NON_AVX})
    set(LLAMA_F16C ${GPT4ALL_ALLOW_NON_AVX})
    set(LLAMA_FMA  ${GPT4ALL_ALLOW_NON_AVX})

    if (BUILD_VARIANT STREQUAL metal)
        set(LLAMA_METAL YES)
    else()
        set(LLAMA_METAL NO)
    endif()

    # Include GGML
    set(LLAMA_K_QUANTS YES)
    include_ggml(llama.cpp-mainline -mainline-${BUILD_VARIANT} ON)

    # Function for preparing individual implementations
    function(prepare_target TARGET_NAME BASE_LIB)
        set(TARGET_NAME ${TARGET_NAME}-${BUILD_VARIANT})
        message(STATUS "Configuring model implementation target ${TARGET_NAME}")
        # Link to ggml/llama
        target_link_libraries(${TARGET_NAME}
            PRIVATE ${BASE_LIB}-${BUILD_VARIANT})
        # Let it know about its build variant
        target_compile_definitions(${TARGET_NAME}
            PRIVATE GGML_BUILD_VARIANT="${BUILD_VARIANT}")
        # Enable IPO if possible
# FIXME: Doesn't work with msvc reliably. See https://github.com/nomic-ai/gpt4all/issues/841
#        set_property(TARGET ${TARGET_NAME}
#                     PROPERTY INTERPROCEDURAL_OPTIMIZATION ${IPO_SUPPORTED})
    endfunction()

    # Add each individual implementations
    add_library(llamamodel-mainline-${BUILD_VARIANT} SHARED
        llamamodel.cpp llmodel_shared.cpp)
    target_compile_definitions(llamamodel-mainline-${BUILD_VARIANT} PRIVATE
        LLAMA_VERSIONS=>=3 LLAMA_DATE=999999)
    prepare_target(llamamodel-mainline llama-mainline)

    if (NOT LLAMA_METAL)
        add_library(gptj-${BUILD_VARIANT} SHARED
            gptj.cpp utils.h utils.cpp llmodel_shared.cpp llmodel_shared.h)
        prepare_target(gptj llama-mainline)

        add_library(bert-${BUILD_VARIANT} SHARED
            bert.cpp utils.h utils.cpp llmodel_shared.cpp llmodel_shared.h)
        target_compile_definitions(bert-${BUILD_VARIANT} PRIVATE LLAMA_VERSIONS=>=3 LLAMA_DATE=999999)
        prepare_target(bert llama-mainline)
    endif()
endforeach()

add_library(llmodel
    llmodel.h llmodel.cpp llmodel_shared.cpp
    llmodel_c.h llmodel_c.cpp
    dlhandle.h
)
target_compile_definitions(llmodel PRIVATE LIB_FILE_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}")

set_target_properties(llmodel PROPERTIES
                              VERSION ${PROJECT_VERSION}
                              SOVERSION ${PROJECT_VERSION_MAJOR})

set(COMPONENT_NAME_MAIN ${PROJECT_NAME})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
