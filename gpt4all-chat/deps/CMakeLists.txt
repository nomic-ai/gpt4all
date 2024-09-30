set(BUILD_SHARED_LIBS OFF)

set(FMT_INSTALL OFF)
add_subdirectory(fmt)

set(QAPPLICATION_CLASS QGuiApplication)
add_subdirectory(SingleApplication)

set(DUCKX_INSTALL OFF)
add_subdirectory(DuckX)
