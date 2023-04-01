@echo off

REM Check if Python is installed
set /p="Checking for python..." <nul
where python >nul 2>&1 
if %ERRORLEVEL% neq 0 (
    echo Python is not installed. Would you like to install Python? [Y/N]
    set /p choice=
    if /i "%choice%" equ "Y" (
        REM Download Python installer
        echo Downloading Python installer...
        powershell -Command "Invoke-WebRequest -Uri 'https://www.python.org/ftp/python/3.10.0/python-3.10.0-amd64.exe' -OutFile 'python.exe'"
        REM Install Python
        echo Installing Python...
        python.exe /quiet /norestart
    ) else (
        echo Please install Python and try again.
        pause
        exit /b 1
    )
) else (
    echo OK
)

REM Check if pip is installed
set /p="Checking for pip..." <nul
python -m pip >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Pip is not installed. Would you like to install pip? [Y/N]
    set /p choice=
    if /i "%choice%" equ "Y" (
        REM Download get-pip.py
        echo Downloading get-pip.py...
        powershell -Command "Invoke-WebRequest -Uri 'https://bootstrap.pypa.io/get-pip.py' -OutFile 'get-pip.py'"
        REM Install pip
        echo Installing pip...
        python get-pip.py
    ) else (
        echo Please install pip and try again.
        pause
        exit /b 1
    )
) else (
    echo OK
)

REM Check if venv module is available
set /p="Checking for venv..." <nul
python -c "import venv" >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo venv module is not available. Would you like to upgrade Python to the latest version? [Y/N]
    set /p choice=
    if /i "%choice%" equ "Y" (
        REM Upgrade Python
        echo Upgrading Python...
        python -m pip install --upgrade pip setuptools wheel
        python -m pip install --upgrade --user python
    ) else (
        echo Please upgrade your Python installation and try again.
        pause
        exit /b 1
    )
) else (
    echo OK
)

REM Create a new virtual environment
set /p="Creating virtual environment ..." <nul
python -m venv env
if %ERRORLEVEL% neq 0 (
    echo Failed to create virtual environment. Please check your Python installation and try again.
    pause
    exit /b 1
) else (
    echo OK
)

REM Activate the virtual environment
set /p="Activating virtual environment ..." <nul
call env\Scripts\activate.bat
echo OK
REM Install the required packages
echo Installing requirements ...
set DS_BUILD_OPS=0
set DS_BUILD_AIO=0
python -m pip install pip --upgrade
python -m pip install -r requirements.txt
python -m pip install deepspeed
if %ERRORLEVEL% neq 0 (
    echo Failed to install required packages. Please check your internet connection and try again.
    pause
    exit /b 1
)

echo Virtual environment created and packages installed successfully.
pause
exit /b 0
