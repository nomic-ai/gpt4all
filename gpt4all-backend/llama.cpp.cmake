cmake_minimum_required(VERSION 3.14)  # for add_link_options and implicit target directories.

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#
# Option list
#
# some of the options here are commented out so they can be set "dynamically" before calling include_ggml()

set(LLAMA_LLAMAFILE_DEFAULT ON)

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
#option(LLAMA_AVX                    "llama: enable AVX"                                     ON)
#option(LLAMA_AVX2                   "llama: enable AVX2"                                    ON)
#option(LLAMA_AVX512                 "llama: enable AVX512"                                  OFF)
#option(LLAMA_AVX512_VBMI            "llama: enable AVX512-VBMI"                             OFF)
#option(LLAMA_AVX512_VNNI            "llama: enable AVX512-VNNI"                             OFF)
#option(LLAMA_FMA                    "llama: enable FMA"                                     ON)
# in MSVC F16C is implied with AVX2/AVX512
#if (NOT MSVC)
#    option(LLAMA_F16C               "llama: enable F16C"                                    ON)
#endif()

if (WIN32)
    set(LLAMA_WIN_VER "0x602" CACHE STRING "llama: Windows Version")
endif()

# 3rd party libs
option(LLAMA_ACCELERATE                      "llama: enable Accelerate framework"               ON)
option(LLAMA_BLAS                            "llama: use BLAS"                                  OFF)
option(LLAMA_LLAMAFILE                       "llama: use llamafile SGEMM"                       ${LLAMA_LLAMAFILE_DEFAULT})
set(LLAMA_BLAS_VENDOR "Generic" CACHE STRING "llama: BLAS library vendor")
#option(LLAMA_CUDA                            "llama: use CUDA"                                  OFF)
option(LLAMA_CUDA_FORCE_DMMV                 "llama: use dmmv instead of mmvq CUDA kernels"     OFF)
option(LLAMA_CUDA_FORCE_MMQ                  "llama: use mmq kernels instead of cuBLAS"         OFF)
set(LLAMA_CUDA_DMMV_X      "32" CACHE STRING "llama: x stride for dmmv CUDA kernels")
set(LLAMA_CUDA_MMV_Y        "1" CACHE STRING "llama: y block size for mmv CUDA kernels")
option(LLAMA_CUDA_F16                        "llama: use 16 bit floats for some calculations"   OFF)
set(LLAMA_CUDA_KQUANTS_ITER "2" CACHE STRING "llama: iters./thread per block for Q2_K/Q6_K")
set(LLAMA_CUDA_PEER_MAX_BATCH_SIZE "128" CACHE STRING
                                             "llama: max. batch size for using peer access")
option(LLAMA_CUDA_NO_PEER_COPY               "llama: do not use peer to peer copies"            OFF)
#option(LLAMA_CLBLAST                         "llama: use CLBlast"                               OFF)
#option(LLAMA_VULKAN                          "llama: use Vulkan"                                OFF)
option(LLAMA_VULKAN_CHECK_RESULTS            "llama: run Vulkan op checks"                      OFF)
option(LLAMA_VULKAN_DEBUG                    "llama: enable Vulkan debug output"                OFF)
option(LLAMA_VULKAN_VALIDATE                 "llama: enable Vulkan validation"                  OFF)
option(LLAMA_VULKAN_RUN_TESTS                "llama: run Vulkan tests"                          OFF)
#option(LLAMA_METAL                           "llama: use Metal"                                 ${LLAMA_METAL_DEFAULT})
option(LLAMA_METAL_NDEBUG                    "llama: disable Metal debugging"                   OFF)
option(LLAMA_METAL_SHADER_DEBUG              "llama: compile Metal with -fno-fast-math"         OFF)
set(LLAMA_METAL_MACOSX_VERSION_MIN "" CACHE STRING
                                             "llama: metal minimum macOS version")
set(LLAMA_METAL_STD "" CACHE STRING          "llama: metal standard version (-std flag)")
#option(LLAMA_KOMPUTE                         "llama: use Kompute"                               OFF)
option(LLAMA_QKK_64                          "llama: use super-block size of 64 for k-quants"   OFF)
set(LLAMA_SCHED_MAX_COPIES  "4" CACHE STRING "llama: max input copies for pipeline parallelism")

# add perf arguments
option(LLAMA_PERF                            "llama: enable perf"                               OFF)

#
# Compile flags
#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

list(APPEND GGML_COMPILE_DEFS GGML_SCHED_MAX_COPIES=${LLAMA_SCHED_MAX_COPIES})

# enable libstdc++ assertions for debug builds
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND GGML_COMPILE_DEFS $<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>)
endif()

if (APPLE AND LLAMA_ACCELERATE)
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

if (LLAMA_BLAS)
    if (LLAMA_STATIC)
        set(BLA_STATIC ON)
    endif()
    if ($(CMAKE_VERSION) VERSION_GREATER_EQUAL 3.22)
        set(BLA_SIZEOF_INTEGER 8)
    endif()

    set(BLA_VENDOR ${LLAMA_BLAS_VENDOR})
    find_package(BLAS)

    if (BLAS_FOUND)
        message(STATUS "BLAS found, Libraries: ${BLAS_LIBRARIES}")

        if ("${BLAS_INCLUDE_DIRS}" STREQUAL "")
            # BLAS_INCLUDE_DIRS is missing in FindBLAS.cmake.
            # see https://gitlab.kitware.com/cmake/cmake/-/issues/20268
            find_package(PkgConfig REQUIRED)
            if (${LLAMA_BLAS_VENDOR} MATCHES "Generic")
                pkg_check_modules(DepBLAS REQUIRED blas)
            elseif (${LLAMA_BLAS_VENDOR} MATCHES "OpenBLAS")
                # As of openblas v0.3.22, the 64-bit is named openblas64.pc
                pkg_check_modules(DepBLAS openblas64)
                if (NOT DepBLAS_FOUND)
                    pkg_check_modules(DepBLAS REQUIRED openblas)
                endif()
            elseif (${LLAMA_BLAS_VENDOR} MATCHES "FLAME")
                pkg_check_modules(DepBLAS REQUIRED blis)
            elseif (${LLAMA_BLAS_VENDOR} MATCHES "ATLAS")
                pkg_check_modules(DepBLAS REQUIRED blas-atlas)
            elseif (${LLAMA_BLAS_VENDOR} MATCHES "FlexiBLAS")
                pkg_check_modules(DepBLAS REQUIRED flexiblas_api)
            elseif (${LLAMA_BLAS_VENDOR} MATCHES "Intel")
                # all Intel* libraries share the same include path
                pkg_check_modules(DepBLAS REQUIRED mkl-sdl)
            elseif (${LLAMA_BLAS_VENDOR} MATCHES "NVHPC")
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

        if (${BLAS_INCLUDE_DIRS} MATCHES "mkl" AND (${LLAMA_BLAS_VENDOR} MATCHES "Generic" OR ${LLAMA_BLAS_VENDOR} MATCHES "Intel"))
            list(APPEND GGML_COMPILE_DEFS GGML_BLAS_USE_MKL)
        endif()

        set(LLAMA_EXTRA_LIBS     ${LLAMA_EXTRA_LIBS}     ${BLAS_LIBRARIES})
        set(LLAMA_EXTRA_INCLUDES ${LLAMA_EXTRA_INCLUDES} ${BLAS_INCLUDE_DIRS})
    else()
        message(WARNING "BLAS not found, please refer to "
        "https://cmake.org/cmake/help/latest/module/FindBLAS.html#blas-lapack-vendors"
        " to set correct LLAMA_BLAS_VENDOR")
    endif()
endif()

if (LLAMA_LLAMAFILE)
    list(APPEND GGML_COMPILE_DEFS GGML_USE_LLAMAFILE)

    set(GGML_HEADERS_LLAMAFILE ${DIRECTORY}/sgemm.h)
    set(GGML_SOURCES_LLAMAFILE ${DIRECTORY}/sgemm.cpp)
endif()

if (LLAMA_QKK_64)
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

    if (LLAMA_CUDA)
        cmake_minimum_required(VERSION 3.17)
        get_property(LANGS GLOBAL PROPERTY ENABLED_LANGUAGES)
        if (NOT CUDA IN_LIST LANGS)
            message(FATAL_ERROR "The CUDA language must be enabled.")
        endif()

        find_package(CUDAToolkit)
        if (NOT CUDAToolkit_FOUND)
            message(FATAL_ERROR "CUDA not found")
        endif()
        message(STATUS "CUDA found")

        set(GGML_HEADERS_CUDA ${DIRECTORY}/ggml-cuda.h)

        file(GLOB GGML_SOURCES_CUDA "${DIRECTORY}/ggml-cuda/*.cu")
        list(APPEND GGML_SOURCES_CUDA "${DIRECTORY}/ggml-cuda.cu")

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_CUDA)
        if (LLAMA_CUDA_FORCE_DMMV)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_FORCE_DMMV)
        endif()
        if (LLAMA_CUDA_FORCE_MMQ)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_FORCE_MMQ)
        endif()
        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_DMMV_X=${LLAMA_CUDA_DMMV_X})
        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_MMV_Y=${LLAMA_CUDA_MMV_Y})
        if (LLAMA_CUDA_F16)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_F16)
        endif()
        list(APPEND GGML_COMPILE_DEFS K_QUANTS_PER_ITERATION=${LLAMA_CUDA_KQUANTS_ITER})
        list(APPEND GGML_COMPILE_DEFS GGML_CUDA_PEER_MAX_BATCH_SIZE=${LLAMA_CUDA_PEER_MAX_BATCH_SIZE})
        if (LLAMA_CUDA_NO_PEER_COPY)
            list(APPEND GGML_COMPILE_DEFS GGML_CUDA_NO_PEER_COPY)
        endif()

        if (LLAMA_STATIC)
            if (WIN32)
                # As of 12.3.1 CUDA Tookit for Windows does not offer a static cublas library
                set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cudart_static CUDA::cublas CUDA::cublasLt)
            else ()
                set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cudart_static CUDA::cublas_static CUDA::cublasLt_static)
            endif()
        else()
            set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cudart CUDA::cublas CUDA::cublasLt)
        endif()

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} CUDA::cuda_driver)

        if (DEFINED CMAKE_CUDA_ARCHITECTURES)
            set(GGML_CUDA_ARCHITECTURES "${CMAKE_CUDA_ARCHITECTURES}")
        else()
            # 52 == lowest CUDA 12 standard
            # 60 == f16 CUDA intrinsics
            # 61 == integer CUDA intrinsics
            # 70 == compute capability at which unrolling a loop in mul_mat_q kernels is faster
            if (LLAMA_CUDA_F16 OR LLAMA_CUDA_DMMV_F16)
                set(GGML_CUDA_ARCHITECTURES "60;61;70") # needed for f16 CUDA intrinsics
            else()
                set(GGML_CUDA_ARCHITECTURES "52;61;70") # lowest CUDA 12 standard + lowest for integer intrinsics
                #set(GGML_CUDA_ARCHITECTURES "") # use this to compile much faster, but only F16 models work
            endif()
        endif()
        message(STATUS "Using CUDA architectures: ${GGML_CUDA_ARCHITECTURES}")
    endif()

    if (LLAMA_CLBLAST)
        find_package(CLBlast)
        if (NOT CLBlast_FOUND)
            message(FATAL_ERROR "CLBlast not found")
        endif()
        message(STATUS "CLBlast found")

        set(GGML_HEADERS_OPENCL ${DIRECTORY}/ggml-opencl.h)
        set(GGML_SOURCES_OPENCL ${DIRECTORY}/ggml-opencl.cpp)

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_CLBLAST)

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} clblast)
    endif()

    if (LLAMA_VULKAN)
        find_package(Vulkan)
        if (NOT Vulkan_FOUND)
            message(FATAL_ERROR "Vulkan not found")
        endif()
        message(STATUS "Vulkan found")

        set(GGML_HEADERS_VULKAN ${DIRECTORY}/ggml-vulkan.h)
        set(GGML_SOURCES_VULKAN ${DIRECTORY}/ggml-vulkan.cpp)

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_VULKAN)

        if (LLAMA_VULKAN_CHECK_RESULTS)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_CHECK_RESULTS)
        endif()

        if (LLAMA_VULKAN_DEBUG)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_DEBUG)
        endif()

        if (LLAMA_VULKAN_VALIDATE)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_VALIDATE)
        endif()

        if (LLAMA_VULKAN_RUN_TESTS)
            list(APPEND GGML_COMPILE_DEFS GGML_VULKAN_RUN_TESTS)
        endif()

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} Vulkan::Vulkan)
    endif()

    set(LLAMA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${DIRECTORY})

    if (LLAMA_KOMPUTE AND NOT GGML_KOMPUTE_ONCE)
        set(GGML_KOMPUTE_ONCE ON PARENT_SCOPE)
        if (NOT EXISTS "${LLAMA_DIR}/kompute/CMakeLists.txt")
            message(FATAL_ERROR "Kompute not found")
        endif()
        message(STATUS "Kompute found")

        list(APPEND GGML_COMPILE_DEFS VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)
        find_package(Vulkan COMPONENTS glslc REQUIRED)
        find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
        if (NOT glslc_executable)
            message(FATAL_ERROR "glslc not found")
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
                    DEPENDS ${LLAMA_DIR}/${source}
                        ${LLAMA_DIR}/kompute-shaders/common.comp
                        ${LLAMA_DIR}/kompute-shaders/op_getrows.comp
                        ${LLAMA_DIR}/kompute-shaders/op_mul_mv_q_n_pre.comp
                        ${LLAMA_DIR}/kompute-shaders/op_mul_mv_q_n.comp
                    COMMAND ${glslc_executable} --target-env=vulkan1.2 -o ${spv_file} ${LLAMA_DIR}/${source}
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

        set(KOMPUTE_OPT_LOG_LEVEL Critical CACHE STRING "Kompute log level")
        add_subdirectory(${LLAMA_DIR}/kompute)

        # Compile our shaders
        compile_shader(SOURCES
            kompute-shaders/op_scale.comp
            kompute-shaders/op_scale_8.comp
            kompute-shaders/op_add.comp
            kompute-shaders/op_addrow.comp
            kompute-shaders/op_mul.comp
            kompute-shaders/op_silu.comp
            kompute-shaders/op_relu.comp
            kompute-shaders/op_gelu.comp
            kompute-shaders/op_softmax.comp
            kompute-shaders/op_norm.comp
            kompute-shaders/op_rmsnorm.comp
            kompute-shaders/op_diagmask.comp
            kompute-shaders/op_mul_mat_mat_f32.comp
            kompute-shaders/op_mul_mat_f16.comp
            kompute-shaders/op_mul_mat_q8_0.comp
            kompute-shaders/op_mul_mat_q4_0.comp
            kompute-shaders/op_mul_mat_q4_1.comp
            kompute-shaders/op_mul_mat_q6_k.comp
            kompute-shaders/op_getrows_f32.comp
            kompute-shaders/op_getrows_f16.comp
            kompute-shaders/op_getrows_q4_0.comp
            kompute-shaders/op_getrows_q4_1.comp
            kompute-shaders/op_getrows_q6_k.comp
            kompute-shaders/op_rope_f16.comp
            kompute-shaders/op_rope_f32.comp
            kompute-shaders/op_cpy_f16_f16.comp
            kompute-shaders/op_cpy_f16_f32.comp
            kompute-shaders/op_cpy_f32_f16.comp
            kompute-shaders/op_cpy_f32_f32.comp
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

    if (LLAMA_KOMPUTE)
        # Add the stamp to the main sources to ensure dependency tracking
        set(GGML_SOURCES_KOMPUTE ${LLAMA_DIR}/ggml-kompute.cpp ${CMAKE_CURRENT_BINARY_DIR}/ggml-kompute.stamp)
        set(GGML_HEADERS_KOMPUTE ${LLAMA_DIR}/ggml-kompute.h)

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_KOMPUTE)

        set(LLAMA_EXTRA_LIBS ${LLAMA_EXTRA_LIBS} kompute)
    endif()

    set(CUDA_CXX_FLAGS "")

    if (LLAMA_CUDA)
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

    if (LLAMA_METAL)
        find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
        find_library(METAL_FRAMEWORK    Metal      REQUIRED)
        find_library(METALKIT_FRAMEWORK MetalKit   REQUIRED)

        message(STATUS "Metal framework found")
        set(GGML_HEADERS_METAL ${DIRECTORY}/ggml-metal.h)
        set(GGML_SOURCES_METAL ${DIRECTORY}/ggml-metal.m)

        list(APPEND GGML_COMPILE_DEFS_PUBLIC GGML_USE_METAL)
        if (LLAMA_METAL_NDEBUG)
            list(APPEND GGML_COMPILE_DEFS GGML_METAL_NDEBUG)
        endif()

        # copy ggml-common.h and ggml-metal.metal to bin directory
        configure_file(${DIRECTORY}/ggml-common.h    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-common.h    COPYONLY)
        configure_file(${DIRECTORY}/ggml-metal.metal ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.metal COPYONLY)

        if (LLAMA_METAL_SHADER_DEBUG)
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
        if (LLAMA_METAL_MACOSX_VERSION_MIN)
            message(STATUS "Adding -mmacosx-version-min=${LLAMA_METAL_MACOSX_VERSION_MIN} flag to metal compilation")
            list(APPEND XC_FLAGS -mmacosx-version-min=${LLAMA_METAL_MACOSX_VERSION_MIN})
        endif()
        if (LLAMA_METAL_STD)
            message(STATUS "Adding -std=${LLAMA_METAL_STD} flag to metal compilation")
            list(APPEND XC_FLAGS -std=${LLAMA_METAL_STD})
        endif()

        add_custom_command(
            OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/default.metallib
            COMMAND xcrun -sdk macosx metal    ${XC_FLAGS} -c ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.metal -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.air
            COMMAND xcrun -sdk macosx metallib                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.air   -o ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/default.metallib
            COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.air
            COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-common.h
            COMMAND rm -f ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ggml-metal.metal
            DEPENDS ${DIRECTORY}/ggml-metal.metal ${DIRECTORY}/ggml-common.h
            COMMENT "Compiling Metal kernels"
            )

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
            if (LLAMA_AVX512)
                list(APPEND ARCH_FLAGS /arch:AVX512)
                # MSVC has no compile-time flags enabling specific
                # AVX512 extensions, neither it defines the
                # macros corresponding to the extensions.
                # Do it manually.
                if (LLAMA_AVX512_VBMI)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:C>:__AVX512VBMI__>)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:CXX>:__AVX512VBMI__>)
                endif()
                if (LLAMA_AVX512_VNNI)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:C>:__AVX512VNNI__>)
                    list(APPEND GGML_COMPILE_DEFS $<$<COMPILE_LANGUAGE:CXX>:__AVX512VNNI__>)
                endif()
            elseif (LLAMA_AVX2)
                list(APPEND ARCH_FLAGS /arch:AVX2)
            elseif (LLAMA_AVX)
                list(APPEND ARCH_FLAGS /arch:AVX)
            endif()
        else()
            if (LLAMA_NATIVE)
                list(APPEND ARCH_FLAGS -march=native)
            endif()
            if (LLAMA_F16C)
                list(APPEND ARCH_FLAGS -mf16c)
            endif()
            if (LLAMA_FMA)
                list(APPEND ARCH_FLAGS -mfma)
            endif()
            if (LLAMA_AVX)
                list(APPEND ARCH_FLAGS -mavx)
            endif()
            if (LLAMA_AVX2)
                list(APPEND ARCH_FLAGS -mavx2)
            endif()
            if (LLAMA_AVX512)
                list(APPEND ARCH_FLAGS -mavx512f)
                list(APPEND ARCH_FLAGS -mavx512bw)
            endif()
            if (LLAMA_AVX512_VBMI)
                list(APPEND ARCH_FLAGS -mavx512vbmi)
            endif()
            if (LLAMA_AVX512_VNNI)
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

    if (LLAMA_CUDA)
        list(APPEND CUDA_CXX_FLAGS ${ARCH_FLAGS})
        list(JOIN CUDA_CXX_FLAGS " " CUDA_CXX_FLAGS_JOINED)  # pass host compiler flags as a single argument
        if (NOT CUDA_CXX_FLAGS_JOINED STREQUAL "")
            list(APPEND CUDA_FLAGS -Xcompiler ${CUDA_CXX_FLAGS_JOINED})
        endif()
        list(APPEND GGML_COMPILE_OPTS "$<$<COMPILE_LANGUAGE:CUDA>:${CUDA_FLAGS}>")
    endif()

    # ggml

    add_library(ggml${SUFFIX} OBJECT
                ${DIRECTORY}/ggml.c
                ${DIRECTORY}/ggml.h
                ${DIRECTORY}/ggml-alloc.c
                ${DIRECTORY}/ggml-alloc.h
                ${DIRECTORY}/ggml-backend.c
                ${DIRECTORY}/ggml-backend.h
                ${DIRECTORY}/ggml-quants.c
                ${DIRECTORY}/ggml-quants.h
                ${GGML_SOURCES_CUDA}      ${GGML_HEADERS_CUDA}
                ${GGML_SOURCES_OPENCL}    ${GGML_HEADERS_OPENCL}
                ${GGML_SOURCES_METAL}     ${GGML_HEADERS_METAL}
                ${GGML_SOURCES_KOMPUTE}   ${GGML_HEADERS_KOMPUTE}
                ${GGML_SOURCES_VULKAN}    ${GGML_HEADERS_VULKAN}
                ${GGML_SOURCES_LLAMAFILE} ${GGML_HEADERS_LLAMAFILE}
                )

    target_include_directories(ggml${SUFFIX} PUBLIC ${DIRECTORY} ${LLAMA_EXTRA_INCLUDES})
    target_compile_features(ggml${SUFFIX} PUBLIC c_std_11) # don't bump

    target_link_libraries(ggml${SUFFIX} PUBLIC Threads::Threads ${LLAMA_EXTRA_LIBS})

    if (BUILD_SHARED_LIBS)
        set_target_properties(ggml${SUFFIX} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()

    # llama

    add_library(llama${SUFFIX} STATIC
                ${DIRECTORY}/llama.cpp
                ${DIRECTORY}/llama.h
                ${DIRECTORY}/unicode.h
                ${DIRECTORY}/unicode.cpp
                ${DIRECTORY}/unicode-data.cpp
                )

    target_include_directories(llama${SUFFIX} PUBLIC ${DIRECTORY})
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
