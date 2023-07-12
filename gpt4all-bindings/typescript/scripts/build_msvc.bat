@echo off

set "BUILD_TYPE=Release"
set "BUILD_DIR=.\build\win-x64-msvc"
set "LIBS_DIR=.\runtimes\win32-x64"

REM Cleanup env
rmdir /s /q %BUILD_DIR%

REM Create directories
mkdir %BUILD_DIR%
mkdir %LIBS_DIR%

REM Build
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -S ..\..\gpt4all-backend -B %BUILD_DIR% -A x64

:BUILD
REM Build the project
cmake --build "%BUILD_DIR%" --parallel --config %BUILD_TYPE%

REM Check the exit code of the build command
if %errorlevel% neq 0 (
    echo Build failed. Retrying...
    goto BUILD
)

mkdir runtimes\win32-x64

REM Copy the DLLs to the desired location
del /F /A /Q %LIBS_DIR%
xcopy /Y "%BUILD_DIR%\bin\%BUILD_TYPE%\*.dll" runtimes\win32-x64\native\

echo Batch script execution completed.
