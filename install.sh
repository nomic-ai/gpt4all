#!/bin/bash

# Check if Python is installed
echo -n "Checking for python..."
if command -v python > /dev/null 2>&1; then
  echo "OK"
else
  read -p "Python is not installed. Would you like to install Python? [Y/N] " choice
  if [ "$choice" = "Y" ] || [ "$choice" = "y" ]; then
    # Download Python installer
    echo "Downloading Python installer..."
    curl -o python.pkg "https://www.python.org/ftp/python/3.10.0/python-3.10.0-macosx11.pkg"
    # Install Python
    echo "Installing Python..."
    sudo installer -pkg python.pkg -target /
  else
    echo "Please install Python and try again."
    exit 1
  fi
fi

# Check if pip is installed
echo -n "Checking for pip..."
if command -v pip > /dev/null 2>&1; then
  echo "OK"
else
  read -p "Pip is not installed. Would you like to install pip? [Y/N] " choice
  if [ "$choice" = "Y" ] || [ "$choice" = "y" ]; then
    # Download get-pip.py
    echo "Downloading get-pip.py..."
    curl -o get-pip.py "https://bootstrap.pypa.io/get-pip.py"
    # Install pip
    echo "Installing pip..."
    sudo python get-pip.py
  else
    echo "Please install pip and try again."
    exit 1
  fi
fi

# Check if venv module is available
echo -n "Checking for venv..."
if python -c "import venv" > /dev/null 2>&1; then
  echo "OK"
else
  read -p "venv module is not available. Would you like to upgrade Python to the latest version? [Y/N] " choice
  if [ "$choice" = "Y" ] || [ "$choice" = "y" ]; then
    # Upgrade Python
    echo "Upgrading Python..."
    python -m pip install --upgrade pip setuptools wheel
    python -m pip install --upgrade --user python
  else
    echo "Please upgrade your Python installation and try again."
    exit 1
  fi
fi

# Create a new virtual environment
echo -n "Creating virtual environment..."
python -m venv env
if [ $? -ne 0 ]; then
  echo "Failed to create virtual environment. Please check your Python installation and try again."
  exit 1
else
  echo "OK"
fi

# Activate the virtual environment
echo -n "Activating virtual environment..."
source env/bin/activate
echo "OK"

# Install the required packages
echo "Installing requirements..."
export DS_BUILD_OPS=0
export DS_BUILD_AIO=0
python -m pip install pip --upgrade
python -m pip install -r requirements.txt
python -m pip install deepspeed
if [ $? -ne 0 ]; then
  echo "Failed to install required packages. Please check your internet connection and try again."
  exit 1
fi

echo "Virtual environment created and packages installed successfully."
