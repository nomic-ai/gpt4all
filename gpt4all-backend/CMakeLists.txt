cmake_minimum_required(VERSION 3.16)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

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
set(LLMODEL_VERSION_MINOR 2)
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
        set(LLAMA_K_QUANTS YES)
        set(LLAMA_METAL YES)
    else()
        set(LLAMA_K_QUANTS NO)
        set(LLAMA_METAL NO)
    endif()

    # Include GGML
    include_ggml(llama.cpp-mainline -mainline-${BUILD_VARIANT} ON)
    if (NOT LLAMA_METAL)
        include_ggml(llama.cpp-230511 -230511-${BUILD_VARIANT} ON)
        include_ggml(llama.cpp-230519 -230519-${BUILD_VARIANT} ON)
    endif()

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

    add_library(replit-mainline-${BUILD_VARIANT} SHARED
    replit.cpp utils.h utils.cpp llmodel_shared.cpp)
    prepare_target(replit-mainline llama-mainline)

    if (NOT LLAMA_METAL)
        add_library(llamamodel-230519-${BUILD_VARIANT} SHARED
            llamamodel.cpp llmodel_shared.cpp)
        target_compile_definitions(llamamodel-230519-${BUILD_VARIANT} PRIVATE
            LLAMA_VERSIONS===2 LLAMA_DATE=230519)
        prepare_target(llamamodel-230519 llama-230519)
        add_library(llamamodel-230511-${BUILD_VARIANT} SHARED
            llamamodel.cpp llmodel_shared.cpp)
        target_compile_definitions(llamamodel-230511-${BUILD_VARIANT} PRIVATE
            LLAMA_VERSIONS=<=1 LLAMA_DATE=230511)
        prepare_target(llamamodel-230511 llama-230511)

        add_library(gptj-${BUILD_VARIANT} SHARED
            gptj.cpp utils.h utils.cpp llmodel_shared.cpp)
        prepare_target(gptj ggml-230511)

        add_library(mpt-${BUILD_VARIANT} SHARED
            mpt.cpp utils.h utils.cpp llmodel_shared.cpp)
        prepare_target(mpt ggml-230511)
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
