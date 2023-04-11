cmake_minimum_required(VERSION 3.16)

project(gpt4all-chat VERSION 0.1 LANGUAGES CXX)

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
)

set_target_properties(chat PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
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

set(COMPONENT_NAME_MAIN "gpt4all-chat")
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
install(TARGETS chat DESTINATION bin COMPONENT ${COMPONENT_NAME_MAIN})

set(CPACK_GENERATOR "IFW")

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
elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    find_program(MACDEPLOYQT macdeployqt HINTS ${_qt_bin_dir})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-mac.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake)
    set(CPACK_IFW_ROOT "~/Qt/Tools/QtInstallerFramework/4.5")
endif()

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_IFW_PACKAGE_NAME "GPT4All Chat")
set(CPACK_IFW_PACKAGE_TITLE "GPT4All Chat Installer")
set(CPACK_IFW_PACKAGE_PUBLISHER "Adam Treat")
set(CPACK_IFW_PRODUCT_URL "https://github.com/manyoso/gpt4all-chat")
set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Aero")
set(CPACK_IFW_PACKAGE_WIZARD_SHOW_PAGE_LIST OFF)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
include(CPack)
include(CPackIFW)
cpack_add_component(${COMPONENT_NAME_MAIN} DOWNLOADED)
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} ESSENTIAL FORCED_INSTALLATION)
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} LICENSES "GPL-3.0 LICENSE" ${CPACK_RESOURCE_FILE_LICENSE})

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
cpack_ifw_add_repository("GPT4AllRepository" URL "http://localhost/linux/repository")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Windows)
cpack_ifw_add_repository("GPT4AllRepository" URL "http://localhost/windows/repository")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
cpack_ifw_add_repository("GPT4AllRepository" URL "http://localhost/mac/repository")
endif()
