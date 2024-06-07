cmake_minimum_required(VERSION 3.21)  # for PROJECT_IS_TOP_LEVEL
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (APPLE)
    option(BUILD_UNIVERSAL "Build a Universal binary on macOS" ON)
else()
    option(LLMODEL_KOMPUTE "llmodel: use Kompute"              ON)
    option(LLMODEL_VULKAN  "llmodel: use Vulkan"               OFF)
    option(LLMODEL_CUDA    "llmodel: use CUDA"                 ON)
    option(LLMODEL_ROCM    "llmodel: use ROCm"                 OFF)
endif()

if (APPLE)
  if (BUILD_UNIVERSAL)
    # Build a Universal binary on macOS
    # This requires that the found Qt library is compiled as Universal binaries.
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  else()
    # Build for the host architecture on macOS
    if (NOT CMAKE_OSX_ARCHITECTURES)
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

set(DIRECTORY llama.cpp-mainline)
include(llama.cpp.cmake)

set(BUILD_VARIANTS)
set(GPTJ_BUILD_VARIANT cpu)
if (APPLE)
    list(APPEND BUILD_VARIANTS metal)
endif()
if (LLMODEL_KOMPUTE)
    list(APPEND BUILD_VARIANTS kompute kompute-avxonly)
    set(GPTJ_BUILD_VARIANT kompute)
else()
    list(PREPEND BUILD_VARIANTS cpu cpu-avxonly)
endif()
if (LLMODEL_VULKAN)
    list(APPEND BUILD_VARIANTS vulkan vulkan-avxonly)
endif()
if (LLMODEL_CUDA)
    if (DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(GGML_CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}")
    endif()

    include(CheckLanguage)
    check_language(CUDA)
    if (NOT CMAKE_CUDA_COMPILER)
        message(WARNING "CUDA Toolkit not found. To build without CUDA, use -DLLMODEL_CUDA=OFF.")
    endif()
    enable_language(CUDA)
    list(APPEND BUILD_VARIANTS cuda cuda-avxonly)
endif()
if (LLMODEL_ROCM)
    enable_language(HIP)
    list(APPEND BUILD_VARIANTS rocm rocm-avxonly)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)

# Go through each build variant
foreach(BUILD_VARIANT IN LISTS BUILD_VARIANTS)
    # Determine flags
    if (BUILD_VARIANT MATCHES avxonly)
        set(GPT4ALL_ALLOW_NON_AVX OFF)
    else()
        set(GPT4ALL_ALLOW_NON_AVX ON)
    endif()
    set(LLAMA_AVX2 ${GPT4ALL_ALLOW_NON_AVX})
    set(LLAMA_F16C ${GPT4ALL_ALLOW_NON_AVX})
    set(LLAMA_FMA  ${GPT4ALL_ALLOW_NON_AVX})

    set(LLAMA_METAL   OFF)
    set(LLAMA_KOMPUTE OFF)
    set(LLAMA_VULKAN  OFF)
    set(LLAMA_CUDA    OFF)
    set(LLAMA_ROCM    OFF)
    if (BUILD_VARIANT MATCHES metal)
        set(LLAMA_METAL   ON)
    elseif (BUILD_VARIANT MATCHES kompute)
        set(LLAMA_KOMPUTE ON)
    elseif (BUILD_VARIANT MATCHES vulkan)
        set(LLAMA_VULKAN  ON)
    elseif (BUILD_VARIANT MATCHES cuda)
        set(LLAMA_CUDA    ON)
    elseif (BUILD_VARIANT MATCHES rocm)
        set(LLAMA_HIPBLAS ON)
    endif()

    # Include GGML
    include_ggml(-mainline-${BUILD_VARIANT})

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

    if (BUILD_VARIANT MATCHES ${GPTJ_BUILD_VARIANT})
        add_library(gptj-${BUILD_VARIANT} SHARED
            gptj.cpp utils.h utils.cpp llmodel_shared.cpp llmodel_shared.h)
        prepare_target(gptj llama-mainline)
    endif()

    if (NOT PROJECT_IS_TOP_LEVEL AND BUILD_VARIANT STREQUAL cuda)
        set(CUDAToolkit_BIN_DIR ${CUDAToolkit_BIN_DIR} PARENT_SCOPE)
    endif()
endforeach()

add_library(llmodel
    llmodel.h llmodel.cpp llmodel_shared.cpp
    llmodel_c.h llmodel_c.cpp
    dlhandle.cpp
)
target_compile_definitions(llmodel PRIVATE LIB_FILE_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}")

set_target_properties(llmodel PROPERTIES
                              VERSION ${PROJECT_VERSION}
                              SOVERSION ${PROJECT_VERSION_MAJOR})

set(COMPONENT_NAME_MAIN ${PROJECT_NAME})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
