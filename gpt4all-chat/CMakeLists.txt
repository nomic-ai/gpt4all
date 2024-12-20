cmake_minimum_required(VERSION 3.25)  # for try_compile SOURCE_FROM_VAR

include(../common/common.cmake)

set(APP_VERSION_MAJOR 3)
set(APP_VERSION_MINOR 6)
set(APP_VERSION_PATCH 2)
set(APP_VERSION_BASE "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")
set(APP_VERSION "${APP_VERSION_BASE}-dev0")

project(gpt4all VERSION ${APP_VERSION_BASE} LANGUAGES CXX C)

if(APPLE)
  option(BUILD_UNIVERSAL "Build a Universal binary on macOS" OFF)
  if(BUILD_UNIVERSAL)
    # Build a Universal binary on macOS
    # This requires that the found Qt library is compiled as Universal binaries.
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
  else()
    # Build for the host architecture on macOS
    set(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
  endif()
endif()

find_package(Python3 3.12 QUIET COMPONENTS Interpreter)

option(GPT4ALL_TEST "Build the tests" ${Python3_FOUND})
option(GPT4ALL_LOCALHOST "Build installer for localhost repo" OFF)
option(GPT4ALL_OFFLINE_INSTALLER "Build an offline installer" OFF)
option(GPT4ALL_SIGN_INSTALL "Sign installed binaries and installers (requires signing identities)" OFF)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    # Enable accurate __cplusplus macro to fix errors in Jinja2Cpp
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>)
endif()


# conftests
function(check_cpp_feature FEATURE_NAME MIN_VALUE)
    message(CHECK_START "Checking for ${FEATURE_NAME} >= ${MIN_VALUE}")
    string(CONCAT SRC
        "#include <version>\n"
        "#if !defined(${FEATURE_NAME}) || ${FEATURE_NAME} < ${MIN_VALUE}\n"
        "#   error \"${FEATURE_NAME} is not defined or less than ${MIN_VALUE}\"\n"
        "#endif\n"
        "int main() { return 0; }\n"
    )
    try_compile(HAS_FEATURE SOURCE_FROM_VAR "test_${FEATURE_NAME}.cpp" SRC)
    if (NOT HAS_FEATURE)
        message(CHECK_FAIL "fail")
        message(FATAL_ERROR
            "The C++ compiler\n  \"${CMAKE_CXX_COMPILER}\"\n"
            "is too old to support ${FEATURE_NAME} >= ${MIN_VALUE}.\n"
            "Please specify a newer compiler via -DCMAKE_C_COMPILER/-DCMAKE_CXX_COMPILER."
        )
    endif()
  message(CHECK_PASS "pass")
endfunction()

# check for monadic operations in std::optional (e.g. transform)
check_cpp_feature("__cpp_lib_optional" "202110L")


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

# Include the binary directory for the generated header file
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Generate a header file with the version number
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)
find_package(Qt6 6.5 COMPONENTS Core HttpServer LinguistTools Pdf Quick QuickDialogs2 Sql Svg REQUIRED)

if (QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)  # generate extra qmldir files on Qt 6.8+
endif()

# Get the Qt6Core target properties
get_target_property(Qt6Core_INCLUDE_DIRS Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(Qt6Core_LIBRARY_RELEASE Qt6::Core LOCATION_RELEASE)

# Find the qmake binary
find_program(QMAKE_EXECUTABLE NAMES qmake qmake6 PATHS ${Qt6Core_INCLUDE_DIRS}/../.. NO_DEFAULT_PATH)

# Get the Qt 6 root directory
get_filename_component(Qt6_ROOT_DIR "${Qt6Core_LIBRARY_RELEASE}" DIRECTORY)
get_filename_component(Qt6_ROOT_DIR "${Qt6_ROOT_DIR}/.." ABSOLUTE)

message(STATUS "qmake binary: ${QMAKE_EXECUTABLE}")
message(STATUS "Qt 6 root directory: ${Qt6_ROOT_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "..." FORCE)
endif()

add_subdirectory(deps)
add_subdirectory(../gpt4all-backend llmodel)

if (GPT4ALL_TEST)
    enable_testing()

    # Llama-3.2-1B model
    set(TEST_MODEL "Llama-3.2-1B-Instruct-Q4_0.gguf")
    set(TEST_MODEL_MD5 "48ff0243978606fdba19d899b77802fc")
    set(TEST_MODEL_PATH "${CMAKE_BINARY_DIR}/resources/${TEST_MODEL}")
    set(TEST_MODEL_URL "https://huggingface.co/bartowski/Llama-3.2-1B-Instruct-GGUF/resolve/main/${TEST_MODEL}")

    # Create a custom command to download the file if it does not exist or if the checksum does not match
    add_custom_command(
        OUTPUT "${TEST_MODEL_PATH}"
        COMMAND ${CMAKE_COMMAND} -E echo "Downloading test model from ${TEST_MODEL_URL} ..."
        COMMAND ${CMAKE_COMMAND} -DURL="${TEST_MODEL_URL}" -DOUTPUT_PATH="${TEST_MODEL_PATH}" -DEXPECTED_MD5="${TEST_MODEL_MD5}" -P "${CMAKE_SOURCE_DIR}/cmake/download_model.cmake"
        DEPENDS "${CMAKE_SOURCE_DIR}/cmake/download_model.cmake"
    )

    # Define a custom target that depends on the downloaded model
    add_custom_target(download_test_model
        DEPENDS "${TEST_MODEL_PATH}"
    )

    add_subdirectory(tests)

    # The 'check' target makes sure the tests and their dependencies are up-to-date before running them
    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure DEPENDS download_test_model chat gpt4all_tests)
endif()

set(CHAT_EXE_RESOURCES)

# Metal shader library
if (APPLE)
    list(APPEND CHAT_EXE_RESOURCES "${GGML_METALLIB}")
endif()

# App icon
if (WIN32)
    list(APPEND CHAT_EXE_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resources/gpt4all.rc")
elseif (APPLE)
    # The MACOSX_BUNDLE_ICON_FILE variable is added to the Info.plist
    # generated by CMake. This variable contains the .icns file name,
    # without the path.
    set(MACOSX_BUNDLE_ICON_FILE gpt4all.icns)

    # And the following tells CMake where to find and install the file itself.
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resources/gpt4all.icns")
    list(APPEND CHAT_EXE_RESOURCES "${APP_ICON_RESOURCE}")
endif()

# Embedding model
set(LOCAL_EMBEDDING_MODEL "nomic-embed-text-v1.5.f16.gguf")
set(LOCAL_EMBEDDING_MODEL_MD5 "a5401e7f7e46ed9fcaed5b60a281d547")
set(LOCAL_EMBEDDING_MODEL_PATH "${CMAKE_BINARY_DIR}/resources/${LOCAL_EMBEDDING_MODEL}")
set(LOCAL_EMBEDDING_MODEL_URL "https://gpt4all.io/models/gguf/${LOCAL_EMBEDDING_MODEL}")
message(STATUS "Downloading embedding model from ${LOCAL_EMBEDDING_MODEL_URL} ...")
file(DOWNLOAD
    "${LOCAL_EMBEDDING_MODEL_URL}"
    "${LOCAL_EMBEDDING_MODEL_PATH}"
    EXPECTED_HASH "MD5=${LOCAL_EMBEDDING_MODEL_MD5}"
)
message(STATUS "Embedding model downloaded to ${LOCAL_EMBEDDING_MODEL_PATH}")
if (APPLE)
    list(APPEND CHAT_EXE_RESOURCES "${LOCAL_EMBEDDING_MODEL_PATH}")
endif()

if (DEFINED GGML_METALLIB)
    set_source_files_properties("${GGML_METALLIB}" PROPERTIES GENERATED ON)
endif()
if (APPLE)
    set_source_files_properties(${CHAT_EXE_RESOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

set(MACOS_SOURCES)
if (APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    list(APPEND MACOS_SOURCES src/macosdock.mm src/macosdock.h)
endif()

qt_add_executable(chat
    src/main.cpp
    src/chat.cpp                  src/chat.h
    src/chatapi.cpp               src/chatapi.h
    src/chatlistmodel.cpp         src/chatlistmodel.h
    src/chatllm.cpp               src/chatllm.h
    src/chatmodel.h               src/chatmodel.cpp
    src/chatviewtextprocessor.cpp src/chatviewtextprocessor.h
    src/codeinterpreter.cpp       src/codeinterpreter.h
    src/database.cpp              src/database.h
    src/download.cpp              src/download.h
    src/embllm.cpp                src/embllm.h
    src/jinja_helpers.cpp         src/jinja_helpers.h
    src/jinja_replacements.cpp    src/jinja_replacements.h
    src/llm.cpp                   src/llm.h
    src/localdocs.cpp             src/localdocs.h
    src/localdocsmodel.cpp        src/localdocsmodel.h
    src/logger.cpp                src/logger.h
    src/modellist.cpp             src/modellist.h
    src/mysettings.cpp            src/mysettings.h
    src/network.cpp               src/network.h
    src/server.cpp                src/server.h
    src/tool.cpp                  src/tool.h
    src/toolcallparser.cpp        src/toolcallparser.h
    src/toolmodel.cpp             src/toolmodel.h
    src/xlsxtomd.cpp              src/xlsxtomd.h
    ${CHAT_EXE_RESOURCES}
    ${MACOS_SOURCES}
)
gpt4all_add_warning_options(chat)

qt_add_qml_module(chat
    URI gpt4all
    VERSION 1.0
    NO_CACHEGEN
    QML_FILES
      main.qml
      qml/AddCollectionView.qml
      qml/AddModelView.qml
      qml/AddGPT4AllModelView.qml
      qml/AddHFModelView.qml
      qml/ApplicationSettings.qml
      qml/ChatDrawer.qml
      qml/ChatCollapsibleItem.qml
      qml/ChatItemView.qml
      qml/ChatMessageButton.qml
      qml/ChatTextItem.qml
      qml/ChatView.qml
      qml/CollectionsDrawer.qml
      qml/HomeView.qml
      qml/LocalDocsSettings.qml
      qml/LocalDocsView.qml
      qml/ModelSettings.qml
      qml/ModelsView.qml
      qml/NetworkDialog.qml
      qml/NewVersionDialog.qml
      qml/PopupDialog.qml
      qml/SettingsView.qml
      qml/StartupDialog.qml
      qml/ConfirmationDialog.qml
      qml/Theme.qml
      qml/ThumbsDownDialog.qml
      qml/Toast.qml
      qml/ToastManager.qml
      qml/MyBusyIndicator.qml
      qml/MyButton.qml
      qml/MyTabButton.qml
      qml/MyCheckBox.qml
      qml/MyComboBox.qml
      qml/MyDialog.qml
      qml/MyDirectoryField.qml
      qml/MyFileDialog.qml
      qml/MyFileIcon.qml
      qml/MyFolderDialog.qml
      qml/MyFancyLink.qml
      qml/MyMenu.qml
      qml/MyMenuItem.qml
      qml/MyMiniButton.qml
      qml/MySettingsButton.qml
      qml/MySettingsDestructiveButton.qml
      qml/MySettingsLabel.qml
      qml/MySettingsStack.qml
      qml/MySettingsTab.qml
      qml/MySlug.qml
      qml/MyTextArea.qml
      qml/MyTextButton.qml
      qml/MyTextField.qml
      qml/MyToolButton.qml
      qml/MyWelcomeButton.qml
    RESOURCES
      icons/antenna_1.svg
      icons/antenna_2.svg
      icons/antenna_3.svg
      icons/caret_down.svg
      icons/caret_right.svg
      icons/changelog.svg
      icons/chat.svg
      icons/check.svg
      icons/close.svg
      icons/copy.svg
      icons/db.svg
      icons/discord.svg
      icons/download.svg
      icons/edit.svg
      icons/eject.svg
      icons/email.svg
      icons/file-doc.svg
      icons/file-docx.svg
      icons/file-md.svg
      icons/file-pdf.svg
      icons/file-txt.svg
      icons/file-xls.svg
      icons/file.svg
      icons/github.svg
      icons/globe.svg
      icons/gpt4all-32.png
      icons/gpt4all-48.png
      icons/gpt4all.svg
      icons/gpt4all_transparent.svg
      icons/home.svg
      icons/image.svg
      icons/info.svg
      icons/left_panel_closed.svg
      icons/left_panel_open.svg
      icons/local-docs.svg
      icons/models.svg
      icons/network.svg
      icons/nomic_logo.svg
      icons/notes.svg
      icons/paperclip.svg
      icons/plus.svg
      icons/plus_circle.svg
      icons/recycle.svg
      icons/regenerate.svg
      icons/search.svg
      icons/send_message.svg
      icons/settings.svg
      icons/stack.svg
      icons/stop_generating.svg
      icons/thumbs_down.svg
      icons/thumbs_up.svg
      icons/trash.svg
      icons/twitter.svg
      icons/up_down.svg
      icons/webpage.svg
      icons/you.svg
)

qt_add_translations(chat
    TS_FILES
    ${CMAKE_SOURCE_DIR}/translations/gpt4all_en_US.ts
    ${CMAKE_SOURCE_DIR}/translations/gpt4all_es_MX.ts
    ${CMAKE_SOURCE_DIR}/translations/gpt4all_zh_CN.ts
    ${CMAKE_SOURCE_DIR}/translations/gpt4all_zh_TW.ts
    ${CMAKE_SOURCE_DIR}/translations/gpt4all_ro_RO.ts
    ${CMAKE_SOURCE_DIR}/translations/gpt4all_it_IT.ts
    ${CMAKE_SOURCE_DIR}/translations/gpt4all_pt_BR.ts
)

set_target_properties(chat PROPERTIES
    WIN32_EXECUTABLE TRUE
)

macro(REPORT_MISSING_SIGNING_CONTEXT)
    message(FATAL_ERROR [=[
        Signing requested but no identity configured.
        Please set the correct env variable or provide the MAC_SIGNING_IDENTITY argument on the command line
        ]=])
endmacro()

if (APPLE)
    set_target_properties(chat PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER gpt4all
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        OUTPUT_NAME gpt4all
    )
    add_dependencies(chat ggml-metal)
endif()

if (APPLE AND GPT4ALL_SIGN_INSTALL)
    if (NOT MAC_SIGNING_IDENTITY)
        if (NOT DEFINED ENV{MAC_SIGNING_CERT_NAME})
            REPORT_MISSING_SIGNING_CONTEXT()
        endif()
        set(MAC_SIGNING_IDENTITY $ENV{MAC_SIGNING_CERT_NAME})
    endif()
    if (NOT MAC_SIGNING_TID)
        if (NOT DEFINED ENV{MAC_NOTARIZATION_TID})
            REPORT_MISSING_SIGNING_CONTEXT()
        endif()
        set(MAC_SIGNING_TID $ENV{MAC_NOTARIZATION_TID})
    endif()

    # Setup MacOS signing for individual binaries
    set_target_properties(chat PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${MAC_SIGNING_TID}
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${MAC_SIGNING_IDENTITY}
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED True
        XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp=http://timestamp.apple.com/ts01  --options=runtime,library"
    )
endif()

target_compile_definitions(chat
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_include_directories(chat PRIVATE src)

# usearch uses the identifier 'slots' which conflicts with Qt's 'slots' keyword
target_compile_definitions(chat PRIVATE QT_NO_SIGNALS_SLOTS_KEYWORDS)

target_include_directories(chat PRIVATE deps/usearch/include
                                        deps/usearch/fp16/include)

target_link_libraries(chat
    PRIVATE Qt6::Core Qt6::HttpServer Qt6::Pdf Qt6::Quick Qt6::Sql Qt6::Svg)
target_link_libraries(chat
    PRIVATE llmodel SingleApplication fmt::fmt duckx::duckx QXlsx jinja2cpp)

if (APPLE)
    target_link_libraries(chat PRIVATE ${COCOA_LIBRARY})
endif()

# -- install --

set(COMPONENT_NAME_MAIN ${PROJECT_NAME})

install(TARGETS chat DESTINATION bin COMPONENT ${COMPONENT_NAME_MAIN})

install(
    TARGETS llmodel
    LIBRARY DESTINATION lib COMPONENT ${COMPONENT_NAME_MAIN}  # .so/.dylib
    RUNTIME DESTINATION bin COMPONENT ${COMPONENT_NAME_MAIN}  # .dll
)

# We should probably iterate through the list of the cmake for backend, but these need to be installed
# to the this component's dir for the finicky qt installer to work
if (LLMODEL_KOMPUTE)
    set(MODEL_IMPL_TARGETS
        llamamodel-mainline-kompute
        llamamodel-mainline-kompute-avxonly
    )
else()
    set(MODEL_IMPL_TARGETS
        llamamodel-mainline-cpu
        llamamodel-mainline-cpu-avxonly
    )
endif()

if (APPLE)
    list(APPEND MODEL_IMPL_TARGETS llamamodel-mainline-metal)
endif()

install(
    TARGETS ${MODEL_IMPL_TARGETS}
    LIBRARY DESTINATION lib COMPONENT ${COMPONENT_NAME_MAIN}  # .so/.dylib
    RUNTIME DESTINATION lib COMPONENT ${COMPONENT_NAME_MAIN}  # .dll
)

if(APPLE AND GPT4ALL_SIGN_INSTALL)
    include(SignMacOSBinaries)
    install_sign_osx(chat)
    install_sign_osx(llmodel)
    foreach(tgt ${MODEL_IMPL_TARGETS})
        install_sign_osx(${tgt})
    endforeach()
endif()

if(WIN32 AND GPT4ALL_SIGN_INSTALL)
    include(SignWindowsBinaries)
    sign_target_windows(chat)
    sign_target_windows(llmodel)
    foreach(tgt ${MODEL_IMPL_TARGETS})
        sign_target_windows(${tgt})
    endforeach()
endif()

if (LLMODEL_CUDA)
    set_property(TARGET llamamodel-mainline-cuda llamamodel-mainline-cuda-avxonly
                 APPEND PROPERTY INSTALL_RPATH "$ORIGIN")

    install(
        TARGETS llamamodel-mainline-cuda
                llamamodel-mainline-cuda-avxonly
        RUNTIME_DEPENDENCY_SET llama-cuda-deps
        LIBRARY DESTINATION lib COMPONENT ${COMPONENT_NAME_MAIN}  # .so/.dylib
        RUNTIME DESTINATION lib COMPONENT ${COMPONENT_NAME_MAIN}  # .dll
    )
    if (WIN32)
        install(
            RUNTIME_DEPENDENCY_SET llama-cuda-deps
            PRE_EXCLUDE_REGEXES "^(nvcuda|api-ms-.*)\\.dll$"
            POST_INCLUDE_REGEXES "(^|[/\\\\])(lib)?(cuda|cublas)" POST_EXCLUDE_REGEXES .
            DIRECTORIES "${CUDAToolkit_BIN_DIR}"
            DESTINATION lib COMPONENT ${COMPONENT_NAME_MAIN}
        )
    endif()
endif()

if (NOT APPLE)
    install(FILES "${LOCAL_EMBEDDING_MODEL_PATH}"
            DESTINATION resources
            COMPONENT ${COMPONENT_NAME_MAIN})
endif()

set(CPACK_GENERATOR "IFW")
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_IFW_VERBOSE ON)

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    find_program(LINUXDEPLOYQT linuxdeployqt HINTS "$ENV{HOME}/dev/linuxdeployqt/build/tools/linuxdeployqt" "$ENV{HOME}/project/linuxdeployqt/bin")
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-linux.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-linux.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-linux.cmake)
    set(CPACK_IFW_ROOT "~/Qt/Tools/QtInstallerFramework/4.6")
    set(CPACK_PACKAGE_FILE_NAME "${COMPONENT_NAME_MAIN}-installer-linux")
    set(CPACK_IFW_TARGET_DIRECTORY "@HomeDir@/${COMPONENT_NAME_MAIN}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    find_program(WINDEPLOYQT windeployqt HINTS ${_qt_bin_dir})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-windows.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-windows.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-windows.cmake)
    set(CPACK_IFW_ROOT "C:/Qt/Tools/QtInstallerFramework/4.6")
    set(CPACK_IFW_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/gpt4all.ico")
    set(CPACK_PACKAGE_FILE_NAME "${COMPONENT_NAME_MAIN}-installer-win64")
    set(CPACK_IFW_TARGET_DIRECTORY "@HomeDir@\\${COMPONENT_NAME_MAIN}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    find_program(MACDEPLOYQT macdeployqt HINTS ${_qt_bin_dir})
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/deploy-qt-mac.cmake.in"
                   "${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake" @ONLY)
    set(CPACK_PRE_BUILD_SCRIPTS ${CMAKE_BINARY_DIR}/cmake/deploy-qt-mac.cmake)
    set(CPACK_IFW_ROOT "~/Qt/Tools/QtInstallerFramework/4.6")
    set(CPACK_IFW_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/gpt4all.icns")
    set(CPACK_PACKAGE_FILE_NAME "${COMPONENT_NAME_MAIN}-installer-darwin")
    set(CPACK_IFW_TARGET_DIRECTORY "@ApplicationsDir@/${COMPONENT_NAME_MAIN}")
    set(CPACK_BUNDLE_NAME ${COMPONENT_NAME_MAIN})
    set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/gpt4all.icns")
endif()

set(CPACK_COMPONENTS_ALL gpt4all)  # exclude development components
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${COMPONENT_NAME_MAIN})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://www.nomic.ai/gpt4all")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/gpt4all-48.png")
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
set(CPACK_PACKAGE_EXECUTABLES "GPT4All")
set(CPACK_CREATE_DESKTOP_LINKS "GPT4All")
set(CPACK_IFW_PACKAGE_NAME "GPT4All")
set(CPACK_IFW_PACKAGE_TITLE "GPT4All Installer")
set(CPACK_IFW_PACKAGE_PUBLISHER "Nomic, Inc.")
set(CPACK_IFW_PRODUCT_URL "https://www.nomic.ai/gpt4all")
set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Aero")
set(CPACK_IFW_PACKAGE_LOGO "${CMAKE_CURRENT_SOURCE_DIR}/icons/gpt4all-48.png")
set(CPACK_IFW_PACKAGE_WINDOW_ICON "${CMAKE_CURRENT_SOURCE_DIR}/icons/gpt4all-32.png")
set(CPACK_IFW_PACKAGE_WIZARD_SHOW_PAGE_LIST OFF)
set(CPACK_IFW_PACKAGE_CONTROL_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/installer_control.qs")

include(InstallRequiredSystemLibraries)
include(CPack)
include(CPackIFW)
if(GPT4ALL_OFFLINE_INSTALLER)
  cpack_add_component(${COMPONENT_NAME_MAIN})
else()
  cpack_add_component(${COMPONENT_NAME_MAIN} DOWNLOADED)
endif()
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} ESSENTIAL FORCED_INSTALLATION)
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} VERSION ${APP_VERSION})
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} LICENSES "MIT LICENSE" ${CPACK_RESOURCE_FILE_LICENSE})
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/installer_component.qs")
cpack_ifw_configure_component(${COMPONENT_NAME_MAIN} REPLACES "gpt4all-chat") #Was used in very earliest prototypes

if (GPT4ALL_LOCALHOST)
    cpack_ifw_add_repository("GPT4AllRepository" URL "http://localhost/repository")
elseif(GPT4ALL_OFFLINE_INSTALLER)
  add_compile_definitions(GPT4ALL_OFFLINE_INSTALLER)
else()
  if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
      cpack_ifw_add_repository("GPT4AllRepository" URL "https://gpt4all.io/installer_repos/linux/repository")
  elseif(${CMAKE_SYSTEM_NAME} MATCHES Windows)
      #To sign the target on windows have to create a batch script add use it as a custom target and then use CPACK_IFW_EXTRA_TARGETS to set this extra target
      cpack_ifw_add_repository("GPT4AllRepository" URL "https://gpt4all.io/installer_repos/windows/repository")
  elseif(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
      cpack_ifw_add_repository("GPT4AllRepository" URL "https://gpt4all.io/installer_repos/mac/repository")
  endif()
endif()
