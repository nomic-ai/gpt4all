#!/bin/bash

# Check if Qt 6 is already installed
if [ -x "$(command -v qmake-qt6)" ]; then
    echo "Qt 6 is already installed."
    exit 0
fi

# Determine the operating system
case "$(uname -s)" in
    Linux*)
        # Linux
        if [ -x "$(command -v apt-get)" ]; then
            package_manager="apt-get"
        elif [ -x "$(command -v yum)" ]; then
            package_manager="yum"
        elif [ -x "$(command -v pacman)" ]; then
            package_manager="pacman"
        else
            echo "Error: Unable to determine the package manager."
            exit 1
        fi
        ;;
    Darwin*)
        # macOS
        package_manager="brew"
        ;;
    CYGWIN*|MINGW32*|MSYS*|MINGW*)
        # Windows
        package_manager="choco"
        ;;
    *)
        echo "Error: Unsupported operating system."
        exit 1
        ;;
esac

# Install Qt 6 using the appropriate package manager
echo "Installing Qt 6..."
case $package_manager in
    "apt-get")
        echo "Using apt-get package manager..."
        sudo apt-get update
        sudo apt-get install qt6-default
        ;;
    "yum")
        echo "Using yum package manager..."
        sudo yum update
        sudo yum install qt6
        ;;
    "pacman")
        echo "Using pacman package manager..."
        sudo pacman -Syu
        sudo pacman -S qt6-base
        ;;
    "brew")
        echo "Using brew package manager..."
        brew update
        brew list qt6 &>/dev/null || brew install qt6
        ;;
    "choco")
        echo "Using Chocolatey package manager..."
        choco install qt6 -y
        ;;
    *)
        echo "Error: Unsupported package manager."
        exit 1
        ;;
esac

# Check if Qt 6 was successfully installed
if [ -x "$(command -v qmake6)" ]; then
    echo "Qt 6 has been successfully installed."
    exit 0
else
    echo "Error: Failed to install Qt 6."
    exit 1
fi
