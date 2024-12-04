set(BUILD_SHARED_LIBS OFF)

set(FMT_INSTALL OFF)
add_subdirectory(fmt)

set(QAPPLICATION_CLASS QApplication)
add_subdirectory(SingleApplication)

set(DUCKX_INSTALL OFF)
add_subdirectory(DuckX)

set(QT_VERSION_MAJOR 6)
add_subdirectory(QXlsx/QXlsx)

# forked dependency of Jinja2Cpp
set(RAPIDJSON_BUILD_DOC                  OFF)
set(RAPIDJSON_BUILD_EXAMPLES             OFF)
set(RAPIDJSON_BUILD_TESTS                OFF)
set(RAPIDJSON_ENABLE_INSTRUMENTATION_OPT OFF)
add_subdirectory(rapidjson)

add_subdirectory(Jinja2Cpp)
