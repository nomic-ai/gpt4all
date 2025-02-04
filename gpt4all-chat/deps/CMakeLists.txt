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
    set(GPT4ALL_PDFIUM_TAG "chromium/6996")
    if (CMAKE_SYSTEM_NAME MATCHES Linux)
        FetchContent_Declare(
            pdfium
            URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-linux-x64.tgz"
            URL_HASH "SHA256=68b381b87efed539f2e33ae1e280304c9a42643a878cc296c1d66a93b0cb4335"
        )
    elseif (CMAKE_SYSTEM_NAME MATCHES Windows)
        if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
            FetchContent_Declare(
                pdfium
                URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-win-x64.tgz"
                URL_HASH "SHA256=83e714c302ceacccf403826d5cb57ea39b77f393d83b8d5781283012774a9378"
            )
        elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|AARCH64|arm64|ARM64)$")
            FetchContent_Declare(
                pdfium
                URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-win-arm64.tgz"
                URL_HASH "SHA256=78e77e871453a4915cbf66fb381b951c9932f88a747c6b2b33c9f27ec2371445"
            )
        endif()
    elseif (CMAKE_SYSTEM_NAME MATCHES Darwin)
        FetchContent_Declare(
            pdfium
            URL "https://github.com/bblanchon/pdfium-binaries/releases/download/${GPT4ALL_PDFIUM_TAG}/pdfium-mac-univ.tgz"
            URL_HASH "SHA256=e7577f3242ff9c1df50025f9615673a43601a201bc51ee4792975f98920793a2"
        )
    endif()

    FetchContent_MakeAvailable(pdfium)
    find_package(PDFium REQUIRED PATHS "${pdfium_SOURCE_DIR}" NO_DEFAULT_PATH)
endif()
