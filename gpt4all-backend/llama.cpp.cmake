cmake_minimum_required(VERSION 3.14)  # for add_link_options and implicit target directories.

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#
# Option list
#
# some of the options here are commented out so they can be set "dynamically" before calling include_ggml()

set(GGML_LLAMAFILE_DEFAULT ON)

# general
option(LLAMA_STATIC                     "llama: static link libraries"                          OFF)
option(LLAMA_NATIVE                     "llama: enable -march=native flag"                      OFF)

# debug
option(LLAMA_ALL_WARNINGS               "llama: enable all compiler warnings"                   ON)
option(LLAMA_ALL_WARNINGS_3RD_PARTY     "llama: enable all compiler warnings in 3rd party libs" OFF)
option(LLAMA_GPROF                      "llama: enable gprof"                                   OFF)

# build
option(LLAMA_FATAL_WARNINGS             "llama: enable -Werror flag"                            OFF)

# instruction set specific
#option(GGML_AVX                     "ggml: enable AVX"                                     ON)
#option(GGML_AVX2                    "ggml: enable AVX2"                                    ON)
#option(GGML_AVX512                  "ggml: enable AVX512"                                  OFF)
#option(GGML_AVX512_VBMI             "ggml: enable AVX512-VBMI"                             OFF)
#option(GGML_AVX512_VNNI             "ggml: enable AVX512-VNNI"                             OFF)
#option(GGML_FMA                     "ggml: enable FMA"                                     ON)
# in MSVC F16C is implied with AVX2/AVX512
#if (NOT MSVC)
#    option(GGML_F16C                "ggml: enable F16C"                                    ON)
#endif()

if (WIN32)
    set(LLAMA_WIN_VER "0x602" CACHE STRING "llama: Windows Version")
endif()

# 3rd party libs
option(GGML_ACCELERATE                      "ggml: enable Accelerate framework"               ON)
option(GGML_BLAS                            "ggml: use BLAS"                                  OFF)
option(GGML_LLAMAFILE                       "ggml: use llamafile SGEMM"                       ${GGML_LLAMAFILE_DEFAULT})
set(GGML_BLAS_VENDOR "Generic" CACHE STRING "ggml: BLAS library vendor")

#option(GGML_CUDA                            "ggml: use CUDA"                                  OFF)
option(GGML_CUDA_FORCE_DMMV                 "ggml: use dmmv instead of mmvq CUDA kernels"     OFF)
option(GGML_CUDA_FORCE_MMQ                  "ggml: use mmq kernels instead of cuBLAS"         OFF)
option(GGML_CUDA_FORCE_CUBLAS               "ggml: always use cuBLAS instead of mmq kernels"  OFF)
set   (GGML_CUDA_DMMV_X   "32" CACHE STRING "ggml: x stride for dmmv CUDA kernels")
set   (GGML_CUDA_MMV_Y     "1" CACHE STRING "ggml: y block size for mmv CUDA kernels")
option(GGML_CUDA_F16                        "ggml: use 16 bit floats for some calculations"   OFF)
set   (GGML_CUDA_KQUANTS_ITER "2" CACHE STRING
                                            "ggml: iters./thread per block for Q2_K/Q6_K")
set   (GGML_CUDA_PEER_MAX_BATCH_SIZE "128" CACHE STRING
                                            "ggml: max. batch size for using peer access")
option(GGML_CUDA_NO_PEER_COPY               "ggml: do not use peer to peer copies"            OFF)
option(GGML_CUDA_NO_VMM                     "ggml: do not try to use CUDA VMM"                OFF)
option(GGML_CUDA_FA_ALL_QUANTS              "ggml: compile all quants for FlashAttention"     OFF)
option(GGML_CUDA_USE_GRAPHS                 "ggml: use CUDA graphs (llama.cpp only)"          OFF)

#option(GGML_HIPBLAS                         "ggml: use hipBLAS"                               OFF)
option(GGML_HIP_UMA                         "ggml: use HIP unified memory architecture"       OFF)
#option(GGML_VULKAN                          "ggml: use Vulkan"                                OFF)
option(GGML_VULKAN_CHECK_RESULTS            "ggml: run Vulkan op checks"                      OFF)
option(GGML_VULKAN_DEBUG                    "ggml: enable Vulkan debug output"                OFF)
option(GGML_VULKAN_VALIDATE                 "ggml: enable Vulkan validation"                  OFF)
option(GGML_VULKAN_RUN_TESTS                "ggml: run Vulkan tests"                          OFF)
#option(GGML_METAL                           "ggml: use Metal"                                 ${GGML_METAL_DEFAULT})
option(GGML_METAL_NDEBUG                    "ggml: disable Metal debugging"                   OFF)
option(GGML_METAL_SHADER_DEBUG              "ggml: compile Metal with -fno-fast-math"         OFF)
set(GGML_METAL_MACOSX_VERSION_MIN "" CACHE STRING
                                            "ggml: metal minimum macOS version")
set(GGML_METAL_STD "" CACHE STRING          "ggml: metal standard version (-std flag)")
#option(GGML_KOMPUTE                        "ggml: use Kompute"                               OFF)
option(GGML_QKK_64                          "ggml: use super-block size of 64 for k-quants"   OFF)
set(GGML_SCHED_MAX_COPIES  "4" CACHE STRING "ggml: max input copies for pipeline parallelism")

# add perf arguments
option(LLAMA_PERF                           "llama: enable perf"                               OFF)

#
# Compile flags
#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

list(APPEND GGML_COMPILE_DEFS GGML_SCHED_MAX_COPIES=${GGML_SCHED_MAX_COPIES})

# enable libstdc++ assertions for debug builds
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND GGML_COMPILE_DEFS $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
endif()

if (APPLE AND GGML_ACCELERATE)
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    if (ACCELERATE_FRAMEWORK)
        message(STATUS "Accelerate framework found")

        list(APPEND GGML_COMPILE_DEFS GGML_USE_ACCELERATE)
        list(APPEND GGML_COMPILE_DEFS ACCELERATE_NEW_LAPACK)
        list(APPEND GGML_COMPILE_DEFS ACCELERATE_LAPACK_ILP64)
        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} ${ACCELERATE_FRAMEWORK})
    else()
        message(WARNING "Accelerate framework not found")
    endif()
endif()

if (GGML_BLAS)
    if (LLAMA_STATIC)
        set(BLA_STATIC ON)
    endif()
    if ($(CMAKE_VERSION) VERSION_GREATER_EQUAL 3.22)
        set(BLA_SIZEOF_INTEGER 8)
    endif()

    set(BLA_VENDOR ${GGML_BLAS_VENDOR})
    find_package(BLAS)

    if (BLAS_FOUND)
        message(STATUS "BLAS found, Libraries: ${BLAS_LIBRARIES}")

        if ("${BLAS_INCLUDE_DIRS}" STREQUAL "")
            # BLAS_INCLUDE_DIRS is missing in FindBLAS.cmake.
            # see https://gitlab.kitware.com/cmake/cmake/-/issues/20268
            find_package(PkgConfig REQUIRED)
            if (${GGML_BLAS_VENDOR} MATCHES "Generic")
                pkg_check_modules(DepBLAS REQUIRED blas)
            elseif (${GGML_BLAS_VENDOR} MATCHES "OpenBLAS")
                # As of openblas v0.3.22, the 64-bit is named openblas64.pc
                pkg_check_modules(DepBLAS openblas64)
                if (NOT DepBLAS_FOUND)
                    pkg_check_modules(DepBLAS REQUIRED openblas)
                endif()
            elseif (${GGML_BLAS_VENDOR} MATCHES "FLAME")
                pkg_check_modules(DepBLAS REQUIRED blis)
            elseif (${GGML_BLAS_VENDOR} MATCHES "ATLAS")
                pkg_check_modules(DepBLAS REQUIRED blas-atlas)
            elseif (${GGML_BLAS_VENDOR} MATCHES "FlexiBLAS")
                pkg_check_modules(DepBLAS REQUIRED flexiblas_api)
            elseif (${GGML_BLAS_VENDOR} MATCHES "Intel")
                # all Intel* libraries share the same include path
                pkg_check_modules(DepBLAS REQUIRED mkl-sdl)
            elseif (${GGML_BLAS_VENDOR} MATCHES "NVHPC")
                # this doesn't provide pkg-config
                # suggest to assign BLAS_INCLUDE_DIRS on your own
                if ("${NVHPC_VERSION}" STREQUAL "")
                    message(WARNING "Better to set NVHPC_VERSION")
                else()
                    set(DepBLAS_FOUND ON)
                    set(DepBLAS_INCLUDE_DIRS "/opt/nvidia/hpc_sdk/${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_PROCESSOR}/${NVHPC_VERSION}/math_libs/include")
                endif()
            endif()
            if (DepBLAS_FOUND)
                set(BLAS_INCLUDE_DIRS ${DepBLAS_INCLUDE_DIRS})
            else()
                message(WARNING "BLAS_INCLUDE_DIRS neither been provided nor been automatically"
                " detected by pkgconfig, trying to find cblas.h from possible paths...")
                find_path(BLAS_INCLUDE_DIRS
                    NAMES cblas.h
                    HINTS
                        /usr/include
                        /usr/local/include
                        /usr/include/openblas
                        /opt/homebrew/opt/openblas/include
                        /usr/local/opt/openblas/include
                        /usr/include/x86_64-linux-gnu/openblas/include
                )
            endif()
        endif()

        message(STATUS "BLAS found, Includes: ${BLAS_INCLUDE_DIRS}")

        list(APPEND GGML_COMPILE_OPTS ${BLAS_LINKER_FLAGS})

        list(APPEND GGML_COMPILE_DEFS GGML_USE_OPENBLAS)

        if (${BLAS_INCLUDE_DIRS} MATCHES "mkl" AND (${GGML_BLAS_VENDOR} MATCHES "Generic" OR ${GGML_BLAS_VENDOR} MATCHES "Intel"))
            list(APPEND GGML_COMPILE_DEFS GGML_BLAS_USE_MKL)
        endif()

        set(LLAMA_EXTRA_LIBS     ${LLAMA_EXTRA_LIBS}     ${BLAS_LIBRARIES})
        set(LLAMA_EXTRA_INCLUDES ${LLAMA_EXTRA_INCLUDES} ${BLAS_INCLUDE_DIRS})
    else()
        message(WARNING "BLAS not found, please refer to "
        "https://cmake.org/cmake/help/latest/module/FindBLAS.html#blas-lapack-vendors"
        " to set correct GGML_BLAS_VENDOR")
    endif()
endif()

if (GGML_LLAMAFILE)
    list(APPEND GGML_COMPILE_DEFS GGML_USE_LLAMAFILE)

    set(GGML_HEADERS_LLAMAFILE ${DIRECTORY}/ggml/src/llamafile/sgemm.h)
    set(GGML_SOURCES_LLAMAFILE ${DIRECTORY}/ggml/src/llamafile/sgemm.cpp)
endif()

if (GGML_QKK_64)
    list(APPEND GGML_COMPILE_DEFS GGML_QKK_64)
endif()

if (LLAMA_PERF)
    list(APPEND GGML_COMPILE_DEFS GGML_PERF)
endif()

function(get_flags CCID CCVER)
    set(C_FLAGS "")
    set(CXX_FLAGS "")

    if (CCID MATCHES "Clang")
        set(C_FLAGS   -Wunreachable-code-break -Wunreachable-code-return)
        set(CXX_FLAGS -Wunreachable-code-break -Wunreachable-code-return -Wmissing-prototypes -Wextra-semi)

        if (
            (CCID STREQUAL "Clang"      AND CCVER VERSION_GREATER_EQUAL 3.8.0) OR
            (CCID STREQUAL "AppleClang" AND CCVER VERSION_GREATER_EQUAL 7.3.0)
        )
            list(APPEND C_FLAGS -Wdouble-promotion)
        endif()
    elseif (CCID STREQUAL "GNU")
        set(C_FLAGS   -Wdouble-promotion)
        set(CXX_FLAGS -Wno-array-bounds)

        if (CCVER VERSION_GREATER_EQUAL 7.1.0)
            list(APPEND CXX_FLAGS -Wno-format-truncation)
        endif()
        if (CCVER VERSION_GREATER_EQUAL 8.1.0)
            list(APPEND CXX_FLAGS -Wextra-semi)
        endif()
    endif()

    set(GF_C_FLAGS   ${C_FLAGS}   PARENT_SCOPE)
    set(GF_CXX_FLAGS ${CXX_FLAGS} PARENT_SCOPE)
endfunction()

if (LLAMA_FATAL_WARNINGS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        list(APPEND C_FLAGS   -Werror)
        list(APPEND CXX_FLAGS -Werror)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(APPEND GGML_COMPILE_OPTS /WX)
    endif()
endif()

if (LLAMA_ALL_WARNINGS)
    if (NOT MSVC)
        list(APPEND WARNING_FLAGS -Wall -Wextra -Wpedantic -Wcast-qual -Wno-unused-function)
        list(APPEND C_FLAGS       -Wshadow -Wstrict-prototypes -Wpointer-arith -Wmissing-prototypes
                                  -Werror=implicit-int -Werror=implicit-function-declaration)
        list(APPEND CXX_FLAGS     -Wmissing-declarations -Wmissing-noreturn)

        list(APPEND C_FLAGS   ${WARNING_FLAGS})
        list(APPEND CXX_FLAGS ${WARNING_FLAGS})

        get_flags(${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION})

        list(APPEND GGML_COMPILE_OPTS "$<$<COMPILE_LANGUAGE:C>:${C_FLAGS};${GF_C_FLAGS}>"
                                      "$<$<COMPILE_LANGUAGE:CXX>:${CXX_FLAGS};${GF_CXX_FLAGS}>")
    else()
        # todo : msvc
        set(C_FLAGS   "")
        set(CXX_FLAGS "")
    endif()
endif()

if (WIN32)
    list(APPEND GGML_COMPILE_DEFS _CRT_SECURE_NO_WARNINGS)

    if (BUILD_SHARED_LIBS)
        set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

# this version of Apple ld64 is buggy
execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${CMAKE_EXE_LINKER_FLAGS} -Wl,-v
    ERROR_VARIABLE output
    OUTPUT_QUIET
)

if (output MATCHES "dyld-1015\.7")
    list(APPEND GGML_COMPILE_DEFS HAVE_BUGGY_APPLE_LINKER)
endif()

# Architecture specific
# TODO: probably these flags need to be tweaked on some architectures
#       feel free to update the Makefile for your architecture and send a pull request or issue
message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
if (MSVC)
    string(TOLOWER "${CMAKE_GENERATOR_PLATFORM}" CMAKE_GENERATOR_PLATFORM_LWR)
    message(STATUS "CMAKE_GENERATOR_PLATFORM: ${CMAKE_GENERATOR_PLATFORM}")
else ()
    set(CMAKE_GENERATOR_PLATFORM_LWR "")
endif ()

if (NOT MSVC)
    if (LLAMA_STATIC)
        list(APPEND GGML_LINK_OPTS -static)
        if (MINGW)
            list(APPEND GGML_LINK_OPTS -static-libgcc -static-libstdc++)
        endif()
    endif()
    if (LLAMA_GPROF)
        list(APPEND GGML_COMPILE_OPTS -pg)
    endif()
endif()

if (MINGW)
    # Target Windows 8 for PrefetchVirtualMemory
    list(APPEND GGML_COMPILE_DEFS _WIN32_WINNT=${LLAMA_WIN_VER})
endif()

#
# POSIX conformance
#

# clock_gettime came in POSIX.1b (1993)
# CLOCK_MONOTONIC came in POSIX.1-2001 / SUSv3 as optional
# posix_memalign came in POSIX.1-2001 / SUSv3
# M_PI is an XSI extension since POSIX.1-2001 / SUSv3, came in XPG1 (1985)
list(APPEND GGML_COMPILE_DEFS _XOPEN_SOURCE=600)

# Somehow in OpenBSD whenever POSIX conformance is specified
# some string functions rely on locale_t availability,
# which was introduced in POSIX.1-2008, forcing us to go higher
if (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    list(REMOVE_ITEM GGML_COMPILE_DEFS _XOPEN_SOURCE=600)
    list(APPEND GGML_COMPILE_DEFS _XOPEN_SOURCE=700)
endif()

# Data types, macros and functions related to controlling CPU affinity and
# some memory allocation are available on Linux through GNU extensions in libc
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND GGML_COMPILE_DEFS _GNU_SOURCE)
endif()

# RLIMIT_MEMLOCK came in BSD, is not specified in POSIX.1,
# and on macOS its availability depends on enabling Darwin extensions
# similarly on DragonFly, enabling BSD extensions is necessary
if (
    CMAKE_SYSTEM_NAME MATCHES "Darwin" OR
    CMAKE_SYSTEM_NAME MATCHES "iOS" OR
    CMAKE_SYSTEM_NAME MATCHES "tvOS" OR
    CMAKE_SYSTEM_NAME MATCHES "DragonFly"
)
    list(APPEND GGML_COMPILE_DEFS _DARWIN_C_SOURCE)
endif()

# alloca is a non-standard interface that is not visible on BSDs when
# POSIX conformance is specified, but not all of them provide a clean way
# to enable it in such cases
if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    list(APPEND GGML_COMPILE_DEFS __BSD_VISIBLE)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    list(APPEND GGML_COMPILE_DEFS _NETBSD_SOURCE)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    list(APPEND GGML_COMPILE_DEFS _BSD_SOURCE)
endif()

function(include_ggml SUFFIX)
    message(STATUS "Configuring ggml implementation target llama${SUFFIX} in ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY}")

    #
    # libraries
    #

    if (GGML_CUDA)
        cmake_minimum_required(VERSION 3.18)  # for CMAKE_CUDA_ARCHITECTURES

        get_property(LANGS GLOBAL PROPERTY ENABLED_LANGUAGES)
        if (NOT CUDA IN_LIST LANGS)
            message(FATAL_ERROR "The CUDA language must be enabled.")
        endif()

        find_package(CUDAToolkit REQUIRED)
        set(CUDAToolkit_BIN_DIR ${CUDAToolkit_BIN_DIR} PARENT_SCOPE)

        if (NOT DEFINED GGML_CUDA_ARCHITECTURES)
            # 52 == lowest CUDA 12 standard
            # 60 == f16 CUDA intrinsics
            # 61 == integer CUDA intrinsics
            # 70 == compute capability at which unrolling a loop in mul_mat_q kernels is faster
            if (GGML_CUDA_F16 OR GGML_CUDA_DMMV_F16)
                set(GGML_CUDA_ARCHITECTURES "60;61;70;75") # needed for f16 CUDA intrinsics
            else()
                set(GGML_CUDA_ARCHITECTURES "52;61;70;75") # lowest CUDA 12 standard + lowest for integer intrinsics
                #set(GGML_CUDA_ARCHITECTURES "OFF") # use this to compile much faster, but only F16 models work
            endif()
        endif()
        message(STATUS "Using CUDA architectures: ${GGML_CUDA_ARCHITECTURES}")

        set(GGML_HEADERS_CUDA ${DIRECTORY}/ggml/include/ggml-cuda.h)
        file(GLOB   GGML_HEADERS_CUDA "${DIRECTORY}/ggml/src/ggml-cuda/*.cuh")
        list(APPEND GGML_HEADERS_CUDA "${DIRECTORY}/ggml/include/ggml-cuda.h")

        file(GLOB   GGML_SOURCES_CUDA "${DIRECTORY}/ggml/src/ggml-cuda/*.cu")
        list(APPEND GGML_SOURCES_CUDA "${DIRECTORY}/ggml/src/ggml-cuda.cu")
        file(GLOB   SRCS "${DIRECTORY}/ggml/src/ggml-cuda/template-instances/fattn-wmma*.cu")
        list(APPEND GGML_SOURCES_CUDA ${SRCS})
        file(GLOB   SRCS "${DIRECTORY}/ggml/src/ggml-cuda/template-instances/mmq*.cu")
        list(APPEND GGML_SOURCES_CUDA ${SRCS})

        if (GGML_CUDA_FA_ALL_QUANTS)
            file(GLOB   SRCS "${DIRECTORY}/ggml/src/ggml-cuda/template-instances/fattn-vec*.cu")
            list(APPEND GGML_SOURCES_CUDA ${SRCS})
            add_compile_definitions(GGML_CUDA_FA_ALL_QUANTS)
        else()
            file(GLOB   SRCS "${DIRECTORY}/ggml/src/ggml-cuda/template-instances/fattn-vec*q4_0-q4_0.cu")
            list(APPEND GGML_SOURCES_CUDA ${SRCS})
            file(GLOB   SRCS "${DIRECTORY}/ggml/src/ggml-cuda/template-instances/fattn-vec*q8_0-q8_0.cu")
            list(APPEND GGML_SOURCES_CUDA ${SRCS})
            file(GLOB   SRCS "${DIRECTORY}/ggml/src/ggml-cuda/template-instances/fattn-vec*f16-f16.cu")
            list(APPEND GGML_SOURCES_CUDA ${SRCS})
        endif()

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_CUDA)

        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_DMMV_X=${GGML_CUDA_DMMV_X})
        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_MMV_Y=${GGML_CUDA_MMV_Y})
        list(APPEND GGML_COMPILE_DEFS K_QUANTS_PER_ITERATION=${GGML_CUDA_KQUANTS_ITER})
        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_PEER_MAX_BATCH_SIZE=${GGML_CUDA_PEER_MAX_BATCH_SIZE})

        if (GGML_CUDA_USE_GRAPHS)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_USE_GRAPHS)
        endif()

        if (GGML_CUDA_FORCE_DMMV)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_FORCE_DMMV)
        endif()

        if (GGML_CUDA_FORCE_MMQ)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_FORCE_MMQ)
        endif()

        if (GGML_CUDA_FORCE_CUBLAS)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_FORCE_CUBLAS)
        endif()

        if (GGML_CUDA_NO_VMM)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_NO_VMM)
        endif()

        if (GGML_CUDA_F16)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_F16)
        endif()

        if (GGML_CUDA_NO_PEER_COPY)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_NO_PEER_COPY)
        endif()

        if (LLAMA_STATIC)
            if (WIN32)
                # As of 12.3.1 CUDA Toolkit for Windows does not offer a static cublas library
                set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cudart_static CUDA::cublas CUDA::cublasLt)
            else ()
                set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cudart_static CUDA::cublas_static CUDA::cublasLt_static)
            endif()
        else()
            set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cudart CUDA::cublas CUDA::cublasLt)
        endif()

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cuda_driver)
    endif()

    if (GGML_VULKAN)
        find_package(Vulkan REQUIRED)

        set(GGML_HEADERS_VULKAN ${DIRECTORY}/ggml/include/ggml-vulkan.h)
        set(GGML_SOURCES_VULKAN ${DIRECTORY}/ggml/src/ggml-vulkan.cpp)

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_VULKAN)

        if (GGML_VULKAN_CHECK_RESULTS)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_CHECK_RESULTS)
        endif()

        if (GGML_VULKAN_DEBUG)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_DEBUG)
        endif()

        if (GGML_VULKAN_VALIDATE)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_VALIDATE)
        endif()

        if (GGML_VULKAN_RUN_TESTS)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_RUN_TESTS)
        endif()

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} Vulkan::Vulkan)
    endif()

    if (GGML_HIPBLAS)
        if ($ENV{ROCM_PATH})
            set(ROCM_PATH $ENV{ROCM_PATH})
        else()
            set(ROCM_PATH /opt/rocm)
        endif()
        list(APPEND CMAKE_PREFIX_PATH ${ROCM_PATH})

        string(REGEX MATCH "hipcc(\.bat)?$" CXX_IS_HIPCC "${CMAKE_CXX_COMPILER}")

        if (CXX_IS_HIPCC AND UNIX)
            message(WARNING "Setting hipcc as the C++ compiler is legacy behavior."
                " Prefer setting the HIP compiler directly. See README for details.")
        else()
            # Forward AMDGPU_TARGETS to CMAKE_HIP_ARCHITECTURES.
            if (AMDGPU_TARGETS AND NOT CMAKE_HIP_ARCHITECTURES)
                set(CMAKE_HIP_ARCHITECTURES ${AMDGPU_ARGETS})
            endif()
            cmake_minimum_required(VERSION 3.21)
            get_property(LANGS GLOBAL PROPERTY ENABLED_LANGUAGES)
            if (NOT HIP IN_LIST LANGS)
                message(FATAL_ERROR "The HIP language must be enabled.")
            endif()
        endif()
        find_package(hip     REQUIRED)
        find_package(hipblas REQUIRED)
        find_package(rocblas REQUIRED)

        message(STATUS "HIP and hipBLAS found")

        set(GGML_HEADERS_ROCM ${DIRECTORY}/ggml/include/ggml-cuda.h)

        file(GLOB GGML_SOURCES_ROCM "${DIRECTORY}/ggml/src/ggml-rocm/*.cu")
        list(APPEND GGML_SOURCES_ROCM "${DIRECTORY}/ggml/src/ggml-rocm.cu")

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_HIPBLAS GGML_USE_CUDA)

        if (GGML_HIP_UMA)
            list(APPEND GGML_COMPILE_DEFS GGML_HIP_UMA)
        endif()

        if (GGML_CUDA_FORCE_DMMV)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_FORCE_DMMV)
        endif()

        if (GGML_CUDA_FORCE_MMQ)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_FORCE_MMQ)
        endif()

        if (GGML_CUDA_NO_PEER_COPY)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_NO_PEER_COPY)
        endif()

        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_DMMV_X=${GGML_CUDA_DMMV_X})
        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_MMV_Y=${GGML_CUDA_MMV_Y})
        list(APPEND GGML_COMPILE_DEFS K_QUANTS_PER_ITERATION=${GGML_CUDA_KQUANTS_ITER})

        if (CXX_IS_HIPCC)
            set_source_files_properties(${GGML_SOURCES_ROCM} PROPERTIES LANGUAGE CXX)
            set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} hip::device)
        else()
            set_source_files_properties(${GGML_SOURCES_ROCM} PROPERTIES LANGUAGE HIP)
        endif()

        if (LLAMA_STATIC)
            message(FATAL_ERROR "Static linking not supported for HIP/ROCm")
        endif()

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} PUBLIC hip::host roc::rocblas roc::hipblas)
    endif()

    set(LLAMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY})

    if (GGML_KOMPUTE AND NOT GGML_KOMPUTE_ONCE)
        set(GGML_KOMPUTE_ONCE ON PARENT_SCOPE)
        if (NOT EXISTS "${LLAMA_DIR}/ggml/src/kompute/CMakeLists.txt")
            message(FATAL_ERROR "Kompute not found")
        endif()
        message(STATUS "Kompute found")

        find_package(Vulkan COMPONENTS glslc)
        if (NOT Vulkan_FOUND)
            message(FATAL_ERROR "Vulkan not found. To build without Vulkan, use -DLLMODEL_KOMPUTE=OFF.")
        endif()
        find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
        if (NOT glslc_executable)
            message(FATAL_ERROR "glslc not found. To build without Vulkan, use -DLLMODEL_KOMPUTE=OFF.")
        endif()

        function(compile_shader)
            set(options)
            set(oneValueArgs)
            set(multiValueArgs SOURCES)
            cmake_parse_arguments(compile_shader "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
            foreach(source ${compile_shader_SOURCES})
                get_filename_component(OP_FILE ${source} NAME)
                set(spv_file ${CMAKE_CURRENT_BINARY_DIR}/${OP_FILE}.spv)
                add_custom_command(
                    OUTPUT ${spv_file}
                    DEPENDS ${LLAMA_DIR}/ggml/src/kompute-shaders/${source}
                        ${LLAMA_DIR}/ggml/src/kompute-shaders/common.comp
                        ${LLAMA_DIR}/ggml/src/kompute-shaders/op_getrows.comp
                        ${LLAMA_DIR}/ggml/src/kompute-shaders/op_mul_mv_q_n_pre.comp
                        ${LLAMA_DIR}/ggml/src/kompute-shaders/op_mul_mv_q_n.comp
                    COMMAND ${glslc_executable} --target-env=vulkan1.2 -o ${spv_file} ${LLAMA_DIR}/ggml/src/kompute-shaders/${source}
                    COMMENT "Compiling ${source} to ${source}.spv"
                    )

                get_filename_component(RAW_FILE_NAME ${spv_file} NAME)
                set(FILE_NAME "shader${RAW_FILE_NAME}")
                string(REPLACE ".comp.spv" ".h" HEADER_FILE ${FILE_NAME})
                string(TOUPPER ${HEADER_FILE} HEADER_FILE_DEFINE)
                string(REPLACE "." "_" HEADER_FILE_DEFINE "${HEADER_FILE_DEFINE}")
                set(OUTPUT_HEADER_FILE "${HEADER_FILE}")
                message(STATUS "${HEADER_FILE} generating ${HEADER_FILE_DEFINE}")
                if(CMAKE_GENERATOR MATCHES "Visual Studio")
                    add_custom_command(
                        OUTPUT ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "/*THIS FILE HAS BEEN AUTOMATICALLY GENERATED - DO NOT EDIT*/" > ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo \"\#ifndef ${HEADER_FILE_DEFINE}\" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo \"\#define ${HEADER_FILE_DEFINE}\" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "namespace kp {" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "namespace shader_data {" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/xxd -i ${RAW_FILE_NAME} >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "}}" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo \"\#endif // define ${HEADER_FILE_DEFINE}\" >> ${OUTPUT_HEADER_FILE}
                        DEPENDS ${spv_file} xxd
                        COMMENT "Converting to hpp: ${FILE_NAME} ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/xxd"
                        )
                else()
                    add_custom_command(
                        OUTPUT ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "/*THIS FILE HAS BEEN AUTOMATICALLY GENERATED - DO NOT EDIT*/" > ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo \"\#ifndef ${HEADER_FILE_DEFINE}\" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo \"\#define ${HEADER_FILE_DEFINE}\" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "namespace kp {" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "namespace shader_data {" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_BINARY_DIR}/bin/xxd -i ${RAW_FILE_NAME} >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo "}}" >> ${OUTPUT_HEADER_FILE}
                        COMMAND ${CMAKE_COMMAND} -E echo \"\#endif // define ${HEADER_FILE_DEFINE}\" >> ${OUTPUT_HEADER_FILE}
                        DEPENDS ${spv_file} xxd
                        COMMENT "Converting to hpp: ${FILE_NAME} ${CMAKE_BINARY_DIR}/bin/xxd"
                        )
                endif()
            endforeach()
        endfunction()

        set(KOMPUTE_OPT_BUILT_IN_VULKAN_HEADER_TAG "v1.3.239" CACHE STRING "Kompute Vulkan headers tag")
        set(KOMPUTE_OPT_LOG_LEVEL Critical CACHE STRING "Kompute log level")
        set(FMT_INSTALL OFF)
        add_subdirectory(${LLAMA_DIR}/ggml/src/kompute)

        # Compile our shaders
        compile_shader(SOURCES
            op_scale.comp
            op_scale_8.comp
            op_add.comp
            op_addrow.comp
            op_mul.comp
            op_silu.comp
            op_relu.comp
            op_gelu.comp
            op_softmax.comp
            op_norm.comp
            op_rmsnorm.comp
            op_diagmask.comp
            op_mul_mat_mat_f32.comp
            op_mul_mat_f16.comp
            op_mul_mat_q8_0.comp
            op_mul_mat_q4_0.comp
            op_mul_mat_q4_1.comp
            op_mul_mat_q6_k.comp
            op_getrows_f32.comp
            op_getrows_f16.comp
            op_getrows_q4_0.comp
            op_getrows_q4_1.comp
            op_getrows_q6_k.comp
            op_rope_f16.comp
            op_rope_f32.comp
            op_cpy_f16_f16.comp
            op_cpy_f16_f32.comp
            op_cpy_f32_f16.comp
            op_cpy_f32_f32.comp
        )

        # Create a custom target for our generated shaders
        add_custom_target(generated_shaders DEPENDS
            shaderop_scale.h
            shaderop_scale_8.h
            shaderop_add.h
            shaderop_addrow.h
            shaderop_mul.h
            shaderop_silu.h
            shaderop_relu.h
            shaderop_gelu.h
            shaderop_softmax.h
            shaderop_norm.h
            shaderop_rmsnorm.h
            shaderop_diagmask.h
            shaderop_mul_mat_mat_f32.h
            shaderop_mul_mat_f16.h
            shaderop_mul_mat_q8_0.h
            shaderop_mul_mat_q4_0.h
            shaderop_mul_mat_q4_1.h
            shaderop_mul_mat_q6_k.h
            shaderop_getrows_f32.h
            shaderop_getrows_f16.h
            shaderop_getrows_q4_0.h
            shaderop_getrows_q4_1.h
            shaderop_getrows_q6_k.h
            shaderop_rope_f16.h
            shaderop_rope_f32.h
            shaderop_cpy_f16_f16.h
            shaderop_cpy_f16_f32.h
            shaderop_cpy_f32_f16.h
            shaderop_cpy_f32_f32.h
        )

        # Create a custom command that depends on the generated_shaders
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ggml-kompute.stamp
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/ggml-kompute.stamp
            DEPENDS generated_shaders
            COMMENT "Ensuring shaders are generated before compiling ggml-kompute.cpp"
        )
    endif()

    if (GGML_KOMPUTE)
        list(APPEND GGML_COMPILE_DEFS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

        # Add the stamp to the main sources to ensure dependency tracking
        set(GGML_SOURCES_KOMPUTE ${LLAMA_DIR}/ggml/src/ggml-kompute.cpp ${CMAKE_CURRENT_BINARY_DIR}/ggml-kompute.stamp)
        set(GGML_HEADERS_KOMPUTE ${LLAMA_DIR}/ggml/include/ggml-kompute.h)

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_KOMPUTE)

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} kompute)
    endif()

    set(CUDA_CXX_FLAGS "")

    if (GGML_CUDA)
        set(CUDA_FLAGS -use_fast_math)

        if (LLAMA_FATAL_WARNINGS)
            list(APPEND CUDA_FLAGS -Werror all-warnings)
        endif()

        if (LLAMA_ALL_WARNINGS AND NOT MSVC)
            set(NVCC_CMD ${CMAKE_CUDA_COMPILER} .c)
            if (NOT CMAKE_CUDA_HOST_COMPILER STREQUAL "")
                list(APPEND NVCC_CMD -ccbin ${CMAKE_CUDA_HOST_COMPILER})
            endif()

            execute_process(
                COMMAND ${NVCC_CMD} -Xcompiler --version
                OUTPUT_VARIABLE CUDA_CCFULLVER
                ERROR_QUIET
            )

            if (NOT CUDA_CCFULLVER MATCHES clang)
                set(CUDA_CCID "GNU")
                execute_process(
                    COMMAND ${NVCC_CMD} -Xcompiler "-dumpfullversion -dumpversion"
                    OUTPUT_VARIABLE CUDA_CCVER
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
            else()
                if (CUDA_CCFULLVER MATCHES Apple)
                    set(CUDA_CCID "AppleClang")
                else()
                    set(CUDA_CCID "Clang")
                endif()
                string(REGEX REPLACE "^.* version ([0-9.]*).*$" "\\1" CUDA_CCVER ${CUDA_CCFULLVER})
            endif()

            message("-- CUDA host compiler is ${CUDA_CCID} ${CUDA_CCVER}")

            get_flags(${CUDA_CCID} ${CUDA_CCVER})
            list(APPEND CUDA_CXX_FLAGS ${CXX_FLAGS} ${GF_CXX_FLAGS})  # This is passed to -Xcompiler later
        endif()

        if (NOT MSVC)
            list(APPEND CUDA_CXX_FLAGS -Wno-pedantic)
        endif()
    endif()

    if (GGML_METAL)
        find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
        find_library(METAL_FRAMEWORK    Metal      REQUIRED)
        find_library(METALKIT_FRAMEWORK MetalKit   REQUIRED)

        message(STATUS "Metal framework found")
        set(GGML_HEADERS_METAL ${DIRECTORY}/ggml/include/ggml-metal.h)
        set(GGML_SOURCES_METAL ${DIRECTORY}/ggml/src/ggml-metal.m)

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_METAL)
        if (GGML_METAL_NDEBUG)
            list(APPEND GGML_COMPILE_DEFS GGML_METAL_NDEBUG)
        endif()

        # copy ggml-common.h and ggml-metal.metal to bin directory
        configure_file(${DIRECTORY}/ggml/src/ggml-common.h    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-common.h    COPYONLY)
        configure_file(${DIRECTORY}/ggml/src/ggml-metal.metal ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.metal COPYONLY)

        if (GGML_METAL_SHADER_DEBUG)
            # custom command to do the following:
            #   xcrun -sdk macosx metal    -fno-fast-math -c ggml-metal.metal -o ggml-metal.air
            #   xcrun -sdk macosx metallib                   ggml-metal.air   -o default.metallib
            #
            # note: this is the only way I found to disable fast-math in Metal. it's ugly, but at least it works
            #       disabling fast math is needed in order to pass tests/test-backend-ops
            # note: adding -fno-inline fixes the tests when using MTL_SHADER_VALIDATION=1
            # note: unfortunately, we have to call it default.metallib instead of ggml.metallib
            #       ref: https://github.com/ggerganov/whisper.cpp/issues/1720
            set(XC_FLAGS -fno-fast-math -fno-inline -g)
        else()
            set(XC_FLAGS -O3)
        endif()

        # Append macOS metal versioning flags
        if (GGML_METAL_MACOSX_VERSION_MIN)
            message(STATUS "Adding -mmacosx-version-min=${GGML_METAL_MACOSX_VERSION_MIN} flag to metal compilation")
            list(APPEND XC_FLAGS -mmacosx-version-min=${GGML_METAL_MACOSX_VERSION_MIN})
        endif()
        if (GGML_METAL_STD)
            message(STATUS "Adding -std=${GGML_METAL_STD} flag to metal compilation")
            list(APPEND XC_FLAGS -std=${GGML_METAL_STD})
        endif()

        set(GGML_METALLIB ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/default.metallib)
        add_custom_command(
            OUTPUT ${GGML_METALLIB}
            COMMAND xcrun -sdk macosx metal    ${XC_FLAGS} -c ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.metal -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.air
            COMMAND xcrun -sdk macosx metallib                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.air   -o ${GGML_METALLIB}
            COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.air
            COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-common.h
            COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.metal
            DEPENDS ${DIRECTORY}/ggml/src/ggml-metal.metal ${DIRECTORY}/ggml/src/ggml-common.h
            COMMENT "Compiling Metal kernels"
            )
        set_source_files_properties(${GGML_METALLIB} DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTIES GENERATED ON)

        add_custom_target(
            ggml-metal ALL
            DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/default.metallib
            )

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS}
            ${FOUNDATION_LIBRARY}
            ${METAL_FRAMEWORK}
            ${METALKIT_FRAMEWORK}
            )
    endif()

    set(ARCH_FLAGS "")

    if (CMAKE_OSX_ARCHITECTURES STREQUAL "arm64" OR CMAKE_GENERATOR_PLATFORM_LWR STREQUAL "arm64" OR
        (NOT CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_GENERATOR_PLATFORM_LWR AND
         CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm.*|ARM64)$"))
        message(STATUS "ARM detected")
        if (MSVC)
            # TODO: arm msvc?
        else()
            check_cxx_compiler_flag(-mfp16-format=ieee COMPILER_SUPPORTS_FP16_FORMAT_I3E)
            if (NOT "${COMPILER_SUPPORTS_FP16_FORMAT_I3E}" STREQUAL "")
                list(APPEND ARCH_FLAGS -mfp16-format=ieee)
            endif()
            if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv6")
                # Raspberry Pi 1, Zero
                list(APPEND ARCH_FLAGS -mfpu=neon-fp-armv8 -mno-unaligned-access)
            endif()
            if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv7")
                if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
                    # Android armeabi-v7a
                    list(APPEND ARCH_FLAGS -mfpu=neon-vfpv4 -mno-unaligned-access -funsafe-math-optimizations)
                else()
                    # Raspberry Pi 2
                    list(APPEND ARCH_FLAGS -mfpu=neon-fp-armv8 -mno-unaligned-access -funsafe-math-optimizations)
                endif()
            endif()
            if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "armv8")
                # Android arm64-v8a
                # Raspberry Pi 3, 4, Zero 2 (32-bit)
                list(APPEND ARCH_FLAGS -mno-unaligned-access)
            endif()
        endif()
    elseif (CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64" OR CMAKE_GENERATOR_PLATFORM_LWR MATCHES "^(x86_64|i686|amd64|x64|win32)$" OR
            (NOT CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_GENERATOR_PLATFORM_LWR AND
             CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|i686|AMD64)$"))
        message(STATUS "x86 detected")
        if (MSVC)
            if (GGML_AVX512)
                list(APPEND ARCH_FLAGS /arch:AVX512)
                # MSVC has no compile-time flags enabling specific
                # AVX512 extensions, neither it defines the
                # macros corresponding to the extensions.
                # Do it manually.
                if (GGML_AVX512_VBMI)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:C>:__AVX512VBMI__>)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:CXX>:__AVX512VBMI__>)
                endif()
                if (GGML_AVX512_VNNI)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:C>:__AVX512VNNI__>)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:CXX>:__AVX512VNNI__>)
                endif()
            elseif (GGML_AVX2)
                list(APPEND ARCH_FLAGS /arch:AVX2)
            elseif (GGML_AVX)
                list(APPEND ARCH_FLAGS /arch:AVX)
            endif()
        else()
            if (GGML_NATIVE)
                list(APPEND ARCH_FLAGS -march=native)
            endif()
            if (GGML_F16C)
                list(APPEND ARCH_FLAGS -mf16c)
            endif()
            if (GGML_FMA)
                list(APPEND ARCH_FLAGS -mfma)
            endif()
            if (GGML_AVX)
                list(APPEND ARCH_FLAGS -mavx)
            endif()
            if (GGML_AVX2)
                list(APPEND ARCH_FLAGS -mavx2)
            endif()
            if (GGML_AVX512)
                list(APPEND ARCH_FLAGS -mavx512f)
                list(APPEND ARCH_FLAGS -mavx512bw)
            endif()
            if (GGML_AVX512_VBMI)
                list(APPEND ARCH_FLAGS -mavx512vbmi)
            endif()
            if (GGML_AVX512_VNNI)
                list(APPEND ARCH_FLAGS -mavx512vnni)
            endif()
        endif()
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc64")
        message(STATUS "PowerPC detected")
        if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc64le")
            list(APPEND ARCH_FLAGS -mcpu=powerpc64le)
        else()
            list(APPEND ARCH_FLAGS -mcpu=native -mtune=native)
            #TODO: Add  targets for Power8/Power9 (Altivec/VSX) and Power10(MMA) and query for big endian systems (ppc64/le/be)
        endif()
    else()
        message(STATUS "Unknown architecture")
    endif()

    list(APPEND GGML_COMPILE_OPTS "$<$<COMPILE_LANGUAGE:CXX>:${ARCH_FLAGS}>")
    list(APPEND GGML_COMPILE_OPTS "$<$<COMPILE_LANGUAGE:C>:${ARCH_FLAGS}>")

    if (GGML_CUDA)
        list(APPEND CUDA_CXX_FLAGS ${ARCH_FLAGS})
        list(JOIN CUDA_CXX_FLAGS " " CUDA_CXX_FLAGS_JOINED)  # pass host compiler flags as a single argument
        if (NOT CUDA_CXX_FLAGS_JOINED STREQUAL "")
            list(APPEND CUDA_FLAGS -Xcompiler ${CUDA_CXX_FLAGS_JOINED})
        endif()
        list(APPEND GGML_COMPILE_OPTS "$<$<COMPILE_LANGUAGE:CUDA>:${CUDA_FLAGS}>")
    endif()

    # ggml

    add_library(ggml${SUFFIX} OBJECT
                ${DIRECTORY}/ggml/include/ggml.h
                ${DIRECTORY}/ggml/include/ggml-alloc.h
                ${DIRECTORY}/ggml/include/ggml-backend.h
                ${DIRECTORY}/ggml/src/ggml.c
                ${DIRECTORY}/ggml/src/ggml-alloc.c
                ${DIRECTORY}/ggml/src/ggml-backend.c
                ${DIRECTORY}/ggml/src/ggml-quants.c
                ${DIRECTORY}/ggml/src/ggml-quants.h
                ${GGML_SOURCES_CUDA}      ${GGML_HEADERS_CUDA}
                ${GGML_SOURCES_METAL}     ${GGML_HEADERS_METAL}
                ${GGML_SOURCES_KOMPUTE}   ${GGML_HEADERS_KOMPUTE}
                ${GGML_SOURCES_VULKAN}    ${GGML_HEADERS_VULKAN}
                ${GGML_SOURCES_ROCM}      ${GGML_HEADERS_ROCM}
                ${GGML_SOURCES_LLAMAFILE} ${GGML_HEADERS_LLAMAFILE}
                ${DIRECTORY}/ggml/src/ggml-aarch64.c
                ${DIRECTORY}/ggml/src/ggml-aarch64.h
                )

    target_include_directories(ggml${SUFFIX} PUBLIC ${DIRECTORY}/ggml/include ${LLAMA_EXTRA_INCLUDES})
    target_include_directories(ggml${SUFFIX} PRIVATE ${DIRECTORY}/ggml/src)
    target_compile_features(ggml${SUFFIX} PUBLIC c_std_11) # don't bump

    target_link_libraries(ggml${SUFFIX} PUBLIC Threads::Threads ${LLAMA_EXTRA_LIBS})

    if (BUILD_SHARED_LIBS)
        set_target_properties(ggml${SUFFIX} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()

    # llama

    add_library(llama${SUFFIX} STATIC
                ${DIRECTORY}/include/llama.h
                ${DIRECTORY}/src/llama.cpp
                ${DIRECTORY}/src/unicode.h
                ${DIRECTORY}/src/unicode.cpp
                ${DIRECTORY}/src/unicode-data.cpp
                )

    target_include_directories(llama${SUFFIX} PUBLIC  ${DIRECTORY}/include ${DIRECTORY}/ggml/include)
    target_include_directories(llama${SUFFIX} PRIVATE ${DIRECTORY}/src)
    target_compile_features   (llama${SUFFIX} PUBLIC cxx_std_11) # don't bump

    target_link_libraries(llama${SUFFIX} PRIVATE
        ggml${SUFFIX}
        ${LLAMA_EXTRA_LIBS}
        )

    if (BUILD_SHARED_LIBS)
        set_target_properties(llama${SUFFIX} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        target_compile_definitions(llama${SUFFIX} PRIVATE LLAMA_SHARED LLAMA_BUILD)
    endif()

    # target options

    set_target_properties(ggml${SUFFIX} llama${SUFFIX} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED true
        C_STANDARD 11
        C_STANDARD_REQUIRED true
        )
    if (GGML_CUDA_ARCHITECTURES)
        set_property(TARGET ggml${SUFFIX} llama${SUFFIX} PROPERTY CUDA_ARCHITECTURES "${GGML_CUDA_ARCHITECTURES}")
    endif()

    target_compile_options(ggml${SUFFIX} PRIVATE "${GGML_COMPILE_OPTS}")
    target_compile_options(llama${SUFFIX} PRIVATE "${GGML_COMPILE_OPTS}")

    target_compile_definitions(ggml${SUFFIX} PRIVATE "${GGML_COMPILE_DEFS}")
    target_compile_definitions(llama${SUFFIX} PRIVATE "${GGML_COMPILE_DEFS}")

    target_compile_definitions(ggml${SUFFIX} PUBLIC "${GGML_COMPILE_DEFS_PUBLIC}")
    target_compile_definitions(llama${SUFFIX} PUBLIC "${GGML_COMPILE_DEFS_PUBLIC}")

    target_link_options(ggml${SUFFIX} PRIVATE "${GGML_LINK_OPTS}")
    target_link_options(llama${SUFFIX} PRIVATE "${GGML_LINK_OPTS}")
endfunction()
