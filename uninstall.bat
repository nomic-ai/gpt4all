@echo off

echo This will uninstall the environment. Are you sure? [Y/N]
set /p choice=
if /i "%choice%" equ "Y" (
    REM Download Python installer
    echo -n 
    set /p="Removing virtual environment..." <nul
    powershell -Command "rm env -y"
    echo OK
    pause
) else (
    echo Please install Python and try again.
    pause
    exit /b 1
)
