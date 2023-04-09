cmake_minimum_required(VERSION 3.16)

project(gpt4all-chat VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)

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
    RESOURCES icons/send_message.svg icons/stop_generating.svg icons/regenerate.svg
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
