cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 0)
set(APP_VERSION_PATCH  0)

set(APP_VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH})

project(Gpt4AllAutoUpdater VERSION ${APP_VERSION} LANGUAGES CXX)

option(BUILD_OFFLINE_UPDATER "Build an offline updater" OFF)

if(BUILD_OFFLINE_UPDATER AND NOT GPT4ALL_INSTALLER_PATH)
    message(FATAL_ERROR "The path to GPT4ALL's installer is required to construct this updater.
Please provide it on the command line using the argument -DGPT4ALL_INSTALLER_PATH=<pth>")
endif()

if(NOT BUILD_OFFLINE_UPDATER AND NOT GPT4ALL_MANIFEST_ENDPOINT)
    message(FATAL_ERROR "The manifest endpoint was not provided, the online installer will be unable to detect updates")
endif()

if(APPLE)
    option(BUILD_UNIVERSAL "Build universal binary on MacOS" OFF)
    if(BUILD_UNIVERSAL)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    else()
        set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
    endif()
endif()

if(APPLE AND BUILD_OFFLINE_UPDATER)
    enable_language(ASM)
    configure_file(src/asm/bake_installer.S.in ${CMAKE_BINARY_DIR}/bake_installer.S @ONLY)
    set(ASSEMBLER_SOURCES ${CMAKE_BINARY_DIR}/bake_installer.S src/Embedded.cxx)
elseif(WIN32 AND BUILD_OFFLINE_UPDATER)
    configure_file(src/resources/installer.rc.in ${CMAKE_BINARY_DIR}/resource.rc @ONLY)
    set(RC_FILES ${CMAKE_BINARY_DIR}/resource.rc src/Resource.cxx)
endif()

if(NOT BUILD_OFFLINE_UPDATER)
    configure_file(src/Download.cxx.in ${CMAKE_BINARY_DIR}/Download.cxx @ONLY)
    set(ONLINE_SOURCES ${CMAKE_BINARY_DIR}/Download.cxx)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(auto_updater_sources
    src/Command.cxx
    src/CommandFactory.cxx
    src/CommandLine.cxx
    src/Downgrade.cxx
    src/Manifest.cxx
    src/Modify.cxx
    src/Package.cxx
    src/State.cxx
    src/Uninstall.cxx
    src/Update.cxx
    src/utils.cxx
    src/main.cxx
    ${ONLINE_SOURCES}
    ${ASSEMBLER_SOURCES}
    ${RC_FILES}
)

add_executable(autoupdater ${auto_updater_sources})
target_link_libraries(autoupdater PRIVATE Qt6::Core Qt6::Network)
target_include_directories(autoupdater PRIVATE include)

if(BUILD_OFFLINE_UPDATER)
    target_compile_definitions(autoupdater PRIVATE OFFLINE)
endif()
