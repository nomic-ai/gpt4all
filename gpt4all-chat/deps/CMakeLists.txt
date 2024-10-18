set(BUILD_SHARED_LIBS OFF)

set(FMT_INSTALL OFF)
add_subdirectory(fmt)

set(QAPPLICATION_CLASS QApplication)
add_subdirectory(SingleApplication)

set(DUCKX_INSTALL OFF)
add_subdirectory(DuckX)

set(QT_VERSION_MAJOR 6)
add_subdirectory(QXlsx/QXlsx)
