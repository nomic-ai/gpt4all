set(BUILD_SHARED_LIBS OFF)

set(FMT_INSTALL OFF)
set(FMT_MODULE ON)
add_subdirectory(fmt)

set(BUILD_TESTING           OFF)
set(QCORO_BUILD_EXAMPLES    OFF)
set(QCORO_WITH_QTDBUS       OFF)
set(QCORO_WITH_QTWEBSOCKETS OFF)
set(QCORO_WITH_QTQUICK      OFF)
set(QCORO_WITH_QML          OFF)
set(QCORO_WITH_QTTEST       OFF)
add_subdirectory(qcoro)
