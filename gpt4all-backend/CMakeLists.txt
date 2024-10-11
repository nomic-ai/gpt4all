cmake_minimum_required(VERSION 3.23)  # for FILE_SET

include(../common/common.cmake)

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

set(CMAKE_CXX_STANDARD 23)
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

set(DIRECTORY deps/llama.cpp-mainline)
include(llama.cpp.cmake)

set(BUILD_VARIANTS)
if (APPLE)
    list(APPEND BUILD_VARIANTS metal)
endif()
if (LLMODEL_KOMPUTE)
    list(APPEND BUILD_VARIANTS kompute kompute-avxonly)
else()
    list(PREPEND BUILD_VARIANTS cpu cpu-avxonly)
endif()
if (LLMODEL_VULKAN)
    list(APPEND BUILD_VARIANTS vulkan vulkan-avxonly)
endif()
if (LLMODEL_CUDA)
    cmake_minimum_required(VERSION 3.18)  # for CMAKE_CUDA_ARCHITECTURES

    # Defaults must be set before enable_language(CUDA).
    # Keep this in sync with the arch list in ggml/src/CMakeLists.txt.
    if (NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        # 52 == lowest CUDA 12 standard
        # 60 == f16 CUDA intrinsics
        # 61 == integer CUDA intrinsics
        # 70 == compute capability at which unrolling a loop in mul_mat_q kernels is faster
        if (GGML_CUDA_F16 OR GGML_CUDA_DMMV_F16)
            set(CMAKE_CUDA_ARCHITECTURES "60;61;70;75") # needed for f16 CUDA intrinsics
        else()
            set(CMAKE_CUDA_ARCHITECTURES "52;61;70;75") # lowest CUDA 12 standard + lowest for integer intrinsics
            #set(CMAKE_CUDA_ARCHITECTURES "OFF") # use this to compile much faster, but only F16 models work
        endif()
    endif()
    message(STATUS "Using CUDA architectures: ${CMAKE_CUDA_ARCHITECTURES}")

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

# Go through each build variant
foreach(BUILD_VARIANT IN LISTS BUILD_VARIANTS)
    # Determine flags
    if (BUILD_VARIANT MATCHES avxonly)
        set(GPT4ALL_ALLOW_NON_AVX OFF)
    else()
        set(GPT4ALL_ALLOW_NON_AVX ON)
    endif()
    set(GGML_AVX2 ${GPT4ALL_ALLOW_NON_AVX})
    set(GGML_F16C ${GPT4ALL_ALLOW_NON_AVX})
    set(GGML_FMA  ${GPT4ALL_ALLOW_NON_AVX})

    set(GGML_METAL   OFF)
    set(GGML_KOMPUTE OFF)
    set(GGML_VULKAN  OFF)
    set(GGML_CUDA    OFF)
    set(GGML_ROCM    OFF)
    if (BUILD_VARIANT MATCHES metal)
        set(GGML_METAL   ON)
    elseif (BUILD_VARIANT MATCHES kompute)
        set(GGML_KOMPUTE ON)
    elseif (BUILD_VARIANT MATCHES vulkan)
        set(GGML_VULKAN  ON)
    elseif (BUILD_VARIANT MATCHES cuda)
        set(GGML_CUDA    ON)
    elseif (BUILD_VARIANT MATCHES rocm)
        set(GGML_HIPBLAS ON)
    endif()

    # Include GGML
    include_ggml(-mainline-${BUILD_VARIANT})

    if (BUILD_VARIANT MATCHES metal)
        set(GGML_METALLIB "${GGML_METALLIB}" PARENT_SCOPE)
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
        src/llamamodel.cpp src/llmodel_shared.cpp)
    gpt4all_add_warning_options(llamamodel-mainline-${BUILD_VARIANT})
    target_compile_definitions(llamamodel-mainline-${BUILD_VARIANT} PRIVATE
        LLAMA_VERSIONS=>=3 LLAMA_DATE=999999)
    target_include_directories(llamamodel-mainline-${BUILD_VARIANT} PRIVATE
        src include/gpt4all-backend
    )
    prepare_target(llamamodel-mainline llama-mainline)

    if (NOT PROJECT_IS_TOP_LEVEL AND BUILD_VARIANT STREQUAL cuda)
        set(CUDAToolkit_BIN_DIR ${CUDAToolkit_BIN_DIR} PARENT_SCOPE)
    endif()
endforeach()

add_library(llmodel
    src/dlhandle.cpp
    src/llmodel.cpp
    src/llmodel_c.cpp
    src/llmodel_shared.cpp
)
gpt4all_add_warning_options(llmodel)
target_sources(llmodel PUBLIC
    FILE_SET public_headers TYPE HEADERS BASE_DIRS include
    FILES include/gpt4all-backend/llmodel.h
          include/gpt4all-backend/llmodel_c.h
          include/gpt4all-backend/sysinfo.h
)
target_compile_definitions(llmodel PRIVATE LIB_FILE_EXT="${CMAKE_SHARED_LIBRARY_SUFFIX}")
target_include_directories(llmodel PRIVATE src include/gpt4all-backend)

set_target_properties(llmodel PROPERTIES
                              VERSION ${PROJECT_VERSION}
                              SOVERSION ${PROJECT_VERSION_MAJOR})

set(COMPONENT_NAME_MAIN ${PROJECT_NAME})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
