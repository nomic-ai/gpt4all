cmake_minimum_required(VERSION 3.16)

if(APPLE)
  # Build a Universal binary on macOS
  # This requires that the found Qt library is compiled as Universal binaries.
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()

set(APP_VERSION_MAJOR 2)
set(APP_VERSION_MINOR 1)
set(APP_VERSION_PATCH 0)
set(APP_VERSION "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")

# Generate a header file with the version number
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

# Include the binary directory for the generated header file
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

project(gpt4all VERSION ${APP_VERSION} LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS Quick Svg REQUIRED)

set(GGML_BUILD_EXAMPLES ON CACHE BOOL "ggml: build examples" FORCE)
add_subdirectory(ggml)

qt_add_executable(chat
    main.cpp
    gptj.h gptj.cpp
    llm.h llm.cpp
    llmodel.h
)

qt_add_qml_module(chat
    URI gpt4all-chat
    VERSION 1.0
    QML_FILES main.qml
    RESOURCES
      icons/send_message.svg
      icons/stop_generating.svg
      icons/regenerate.svg
      icons/logo.svg
      icons/copy.svg
      icons/settings.svg
      icons/logo-16.png
      icons/logo-32.png
      icons/logo-48.png
      icons/logo-64.png
      icons/logo-256.png
      icons/favicon.ico
      icons/favicon.icns
)

set_target_properties(chat PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER gpt4all.chat
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_compile_definitions(chat
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(chat
    PRIVATE Qt6::Quick Qt6::Svg)
target_link_libraries(chat
    PRIVATE ggml ggml_utils)

set(COMPONENT_NAME_MAIN ${PROJECT_NAME})
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
install(TARGETS chat DESTINATION bin COMPONENT ${COMPONENT_NAME_MAIN})

set(CPACK_GENERATOR "IFW")

set(BUNDLED_MODEL_NAME "ggml-gpt4all-j-v1.2-jazzy.bin")

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    set(LINUXDEPLOYQT "/home/atreat/dev/linuxdeployqt/build/tools/linuxdeployqt/linuxdeployqt")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-linux.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-linux.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-linux.cmake)
    set(CPACK_IFW_ROOT "~/Qt/Tools/QtInstallerFramework/4.5")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    find_program(WINDEPLOYQT windeployqt HINTS ${_qt_bin_dir})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-windows.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-windows.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-windows.cmake)
    set(CPACK_IFW_ROOT "C:/Qt/Tools/QtInstallerFramework/4.5")
    set(CPACK_IFW_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/favicon.ico")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    find_program(MACDEPLOYQT macdeployqt HINTS ${_qt_bin_dir})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-mac.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake)
    set(CPACK_IFW_ROOT "~/Qt/Tools/QtInstallerFramework/4.5")
    set(CPACK_IFW_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/favicon.icns")
endif()

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://gpt4all.io")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/logo-48.png")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_PACKAGE_EXECUTABLES "GPT4All")
set(CPACK_CREATE_DESKTOP_LINKS "GPT4All")
set(CPACK_IFW_PACKAGE_NAME "GPT4All")
set(CPACK_IFW_PACKAGE_TITLE "GPT4All Installer")
set(CPACK_IFW_PACKAGE_PUBLISHER "Nomic, Inc.")
set(CPACK_IFW_PRODUCT_URL "https://gpt4all.io")
set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Aero")
set(CPACK_IFW_PACKAGE_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/icons/logo-48.png")
set(CPACK_IFW_PACKAGE_WINDOW_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/logo-32.png")
set(CPACK_IFW_PACKAGE_WIZARD_SHOW_PAGE_LIST OFF)

include(CPack)
include(CPackIFW)
cpack_add_component(${COMPONENT_NAME_MAIN} DOWNLOADED)
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} ESSENTIAL FORCED_INSTALLATION)
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} VERSION ${APP_VERSION})
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} LICENSES "GPL-3.0 LICENSE" ${CPACK_RESOURCE_FILE_LICENSE})
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/installerscript.qs")
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} REPLACES "gpt4all-chat") #Was used in very earliest prototypes

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
cpack_ifw_add_repository("GPT4AllRepository" URL "https://gpt4all.io/installer_repos/linux/repository")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Windows)
#To sign the target on windows have to create a batch script add use it as a custom target and then use CPACK_IFW_EXTRA_TARGETS to set this extra target
cpack_ifw_add_repository("GPT4AllRepository" URL "https://gpt4all.io/installer_repos/windows/repository")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
#Use this in future https://cmake.org/cmake/help/latest/cpack_gen/ifw.html#variable:CPACK_IFW_PACKAGE_SIGNING_IDENTITY
cpack_ifw_add_repository("GPT4AllRepository" URL "https://gpt4all.io/installer_repos/mac/repository")
endif()
