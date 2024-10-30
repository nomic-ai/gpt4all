# Setup Generic CPack options
include(InstallRequiredSystemLibraries)
set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_PACKAGE_NAME "gpt4all")
set(CPACK_PACKAGE_VERSION ${APP_VERSION_BASE})
set(CPACK_PACKAGE_VERSION_MAJOR )
set(CPACK_PACKAGE_VERSION_MINOR )
set(CPACK_PACKAGE_VERSION_PATCH )
set(CPACK_PACKAGE_VENDOR "nomic")
set(CPACK_PACKAGE_DESCRIPTION
        "GPT4All runs large language models (LLMs) privately on everyday desktops & laptops.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${COMPONENT_NAME_MAIN})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.nomic.ai/gpt4all")
set(CPACK_PACKAGE_EXECUTABLES chat;gpt4all)
set(CPACK_CREATE_DESKTOP_LINKS "gpt4all")
# TODO: Is there another intro message we want?
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/../README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/../LICENSE.txt")
# TODO: ask Adam if there's a better email to use here
set(CPACK_PACKAGE_CONTACT "adam@nomic.ai")

# Why are we creating components here? There's only one... gpt4all is installed monolithically
# unless we want to start vendoring models for the downloaded version?
if(GPT4ALL_OFFLINE_INSTALLER)
  cpack_add_component(${COMPONENT_NAME_MAIN})
else()
  cpack_add_component(${COMPONENT_NAME_MAIN} DOWNLOADED)
endif()

# Setup platform specific CPack options

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    find_program(LINUXDEPLOYQT linuxdeployqt HINTS "$ENV{HOME}/dev/linuxdeployqt/build/tools/linuxdeployqt" "$ENV{HOME}/project/linuxdeployqt/bin")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-linux.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-linux.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-linux.cmake)
    # Still using IFW on Linux
    set(CPACK_GENERATOR "IFW")
    set(CPACK_IFW_VERBOSE ON)
    set(CPACK_IFW_PACKAGE_NAME "GPT4All")
    set(CPACK_IFW_PACKAGE_TITLE "GPT4All Installer")
    set(CPACK_IFW_PACKAGE_PUBLISHER "Nomic, Inc.")
    set(CPACK_IFW_PRODUCT_URL "https://www.nomic.ai/gpt4all")
    set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Aero")
    set(CPACK_IFW_PACKAGE_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/icons/gpt4all-48.png")
    set(CPACK_IFW_PACKAGE_WINDOW_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/gpt4all-32.png")
    set(CPACK_IFW_PACKAGE_WIZARD_SHOW_PAGE_LIST OFF)
    set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/installer_control.qs")
    set(CPACK_IFW_ROOT "~/Qt/Tools/QtInstallerFramework/4.6")
    set(CPACK_PACKAGE_FILE_NAME "${COMPONENT_NAME_MAIN}-installer-linux")
    set(CPACK_IFW_TARGET_DIRECTORY "@HomeDir@/${COMPONENT_NAME_MAIN}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    find_program(WINDEPLOYQT windeployqt HINTS ${_qt_bin_dir})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-windows.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-windows.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-windows.cmake)
    set(CPACK_PACKAGE_FILE_NAME "${COMPONENT_NAME_MAIN}-installer-win64")
    set(CPACK_GENERATOR "INNOSETUP")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/gpt4all-48.bmp")
    # Todo - extend this to all languages we want to support
    set(GPT4ALL_DIST_LANGUAGES english)
    set(GPT4ALL_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/icons/gpt4all-48.ico")
    if (CMAKE_SIZEOF_VOID_P GREATER 4)
      set(GPT4ALL_INSTALLER_ARCH x64)
  else()
      set(GPT4ALL_INSTALLER_ARCH x86)
  endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    find_program(MACDEPLOYQT macdeployqt HINTS ${_qt_bin_dir})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-mac.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake" @ONLY)
    # set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake)
    set(CPACK_PRE_BUILD_SCRIPTS "")
    set(CPACK_PACKAGE_FILE_NAME "${COMPONENT_NAME_MAIN}-installer-darwin")
    set(CPACK_BUNDLE_NAME ${COMPONENT_NAME_MAIN})
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/gpt4all.icns")
    set(CPACK_BUNDLE_ICON ${CPACK_PACKAGE_ICON})
    set(CPACK_PACKAGING_INSTALL_PREFIX "gpt4all")
    set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
    set(CPACK_DMG_DISABLE_APPLICATIONS_SYMLINK ON)
    set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
endif()

# Setup vars for CPack option config

# Configure CPack options file (this sets up generator specfic behavior)
configure_file(${CMAKE_SOURCE_DIR}/cmake/gpt4allCPackOptions.cmake.in ${CMAKE_BINARY_DIR}/gpt4allCPackOptions.cmake)
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_BINARY_DIR}/gpt4allCPackOPtions.cmake)

# Setup Cpack
include(CPack)
if(LINUX)
    include(CPackIFW)
    cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} ESSENTIAL FORCED_INSTALLATION)
    cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} VERSION ${APP_VERSION})
    cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} LICENSES "MIT LICENSE" ${CPACK_RESOURCE_FILE_LICENSE})
    cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/installer_component.qs")
    cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} REPLACES "gpt4all-chat") #Was used in very earliest prototypes
endif()
