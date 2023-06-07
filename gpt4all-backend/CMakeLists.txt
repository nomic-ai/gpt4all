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
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
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

    # Include GGML
    include_ggml(llama.cpp-mainline -mainline-${BUILD_VARIANT} ON)
    include_ggml(llama.cpp-230511 -230511-${BUILD_VARIANT} ON)
    include_ggml(llama.cpp-230519 -230519-${BUILD_VARIANT} ON)

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

    add_library(replit-${BUILD_VARIANT} SHARED
        replit.cpp utils.h utils.cpp llmodel_shared.cpp)
    prepare_target(replit ggml-230511)
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


# build tests
option(LLMODEL_BUILD_TESTS "Build llmodel tests" OFF)
if (LLMODEL_BUILD_TESTS)
    cmake_policy(SET CMP0135 NEW)

    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.13.0.tar.gz)
    # for Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(WRITE "${CMAKE_BINARY_DIR}/null.cpp" "")
    add_executable(llmodel_test "${CMAKE_BINARY_DIR}/null.cpp")
    target_link_libraries(llmodel_test PUBLIC GTest::gtest_main llmodel)

    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/tests.json TESTS_JSON)
    string(JSON TESTS_JSON_LEN LENGTH ${TESTS_JSON})
    math(EXPR TESTS_JSON_MAXIDX "${TESTS_JSON_LEN} - 1")
    message(STATUS "${TESTS_JSON_LEN} llmodel implementation tests found")

    foreach(IDX RANGE ${TESTS_JSON_MAXIDX})
        string(JSON TEST_NAME GET ${TESTS_JSON} ${IDX} name)
        string(JSON TEST_DW_URL GET ${TESTS_JSON} ${IDX} url)
        string(JSON TEST_RESPONSE GET ${TESTS_JSON} ${IDX} test_response)

        message(STATUS "Configuring model implementation test ${TEST_NAME}")

        set(TEST_MODEL ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_${TEST_NAME}.bin)

        if (NOT EXISTS ${TEST_MODEL})
            file(DOWNLOAD
                "${TEST_DW_URL}"
                "${TEST_MODEL}"
                SHOW_PROGRESS)
        endif()

        set(TEST_TARGET llmodel_test_${TEST_NAME})
        set(TEST_INCLUDE "${CMAKE_BINARY_DIR}/test_${TEST_NAME}/")

        # Stuff that can't be passed via target_compile_definitions() for one or another reason
        file(WRITE ${TEST_INCLUDE}/macros.h "#define GET_SUITE_NAME(PREFIX) PREFIX ## ${TEST_NAME}\n")
        file(APPEND ${TEST_INCLUDE}/macros.h "constexpr char LLMODEL_TEST_RESPONSE[] = R\"(${TEST_RESPONSE})\";\n")
        file(APPEND ${TEST_INCLUDE}/macros.h "#define LLMODEL_TEST_RESPONSE_LEN (sizeof(LLMODEL_TEST_RESPONSE)-1)\n")

        add_library(${TEST_TARGET} SHARED
            llmodel_cpp_test.cpp
            llmodel_c_test.cpp)
        target_compile_definitions(${TEST_TARGET} PRIVATE
            LLMODEL_TEST_NAME=${TEST_NAME}
            LLMODEL_TEST_MODEL="${TEST_MODEL}")
        target_link_libraries(${TEST_TARGET} PUBLIC GTest::gtest_main llmodel)
        target_include_directories(${TEST_TARGET} PRIVATE ${TEST_INCLUDE})

        target_link_libraries(llmodel_test PUBLIC ${TEST_TARGET})
    endforeach()
endif()
