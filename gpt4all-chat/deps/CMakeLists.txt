include(FetchContent)


set(BUILD_SHARED_LIBS OFF)

set(FMT_INSTALL OFF)
add_subdirectory(fmt)

set(QAPPLICATION_CLASS QApplication)
add_subdirectory(SingleApplication)

set(DUCKX_INSTALL OFF)
add_subdirectory(DuckX)

set(QT_VERSION_MAJOR 6)
add_subdirectory(QXlsx/QXlsx)

if (NOT GPT4ALL_USING_QTPDF)
    # If we do not use QtPDF, we need to get PDFium.
    set(GPT4ALL_PDFIUM_TAG "chromium/6954")
    if (CMAKE_SYSTEM_NAME MATCHES Linux)
        FetchContent_Declare(
            pdfium
            URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-linux-x64.tgz"
            URL_HASH "SHA256=69917fd9543befc6c806254aff6c8a604d9e7cd3999a3e70fc32b8690d372da2"
        )
    elseif (CMAKE_SYSTEM_NAME MATCHES Windows)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            FetchContent_Declare(
                pdfium
                URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-win-x64.tgz"
                URL_HASH "SHA256=62ecac78fbaf658457beaffcc05eb147f493d435a2e1309e6a731808b4e80d38"
            )
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|AARCH64|arm64|ARM64)$")
            FetchContent_Declare(
                pdfium
                URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-win-arm64.tgz"
                URL_HASH "SHA256=a0b69014467f2b9824776c064920bc95359c9ba0d88793bdda1894a0f22206f8"
            )
        endif()
    elseif (CMAKE_SYSTEM_NAME MATCHES Darwin)
        FetchContent_Declare(
            pdfium
            URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-mac-univ.tgz"
            URL_HASH "SHA256=7442f1dc6bef90898b2b7bd38dbec369ddd81bbf66c1c5aac3a1b60e107098f9"
        )
    endif()

    FetchContent_MakeAvailable(pdfium)
    find_package(PDFium REQUIRED PATHS "${pdfium_SOURCE_DIR}" NO_DEFAULT_PATH)
endif()
