# Building gpt4all-chat from source

Depending upon your operating system, there are many ways that Qt is distributed.
Here is the recommended method for getting the Qt dependency installed to setup and build
gpt4all-chat from source.

## Prerequisites

You will need a compiler. On Windows, you should install Visual Studio with the C++ Development components. On macOS, you will need the full version of Xcode&mdash;Xcode Command Line Tools lacks certain required tools. On Linux, you will need a GCC or Clang toolchain with C++ support.

On Windows and Linux, building GPT4All with full GPU support requires the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) and the latest [CUDA Toolkit](https://developer.nvidia.com/cuda-downloads).

## Note for Linux users

Linux users may install Qt via their distro's official packages instead of using the Qt installer. You need at least Qt 6.5, with support for QPdf and the Qt HTTP Server. You may build from the CLI using CMake and Ninja, or with Qt Creator as described later in this document.

On Arch Linux, this looks like:
```
sudo pacman -S --needed cmake gcc ninja qt6-5compat qt6-base qt6-declarative qt6-httpserver qt6-svg qtcreator
```

On Ubuntu 23.04, this looks like:
```
sudo apt install cmake g++ libgl-dev libqt6core5compat6 ninja-build qml6-module-qt5compat-graphicaleffects qt6-base-private-dev qt6-declarative-dev qt6-httpserver-dev qt6-svg-dev qtcreator
```

On Fedora 39, this looks like:
```
sudo dnf install cmake gcc-c++ ninja-build qt-creator qt5-qtgraphicaleffects qt6-qt5compat qt6-qtbase-private-devel qt6-qtdeclarative-devel qt6-qthttpserver-devel qt6-qtsvg-devel
```

## Download Qt

- Go to https://login.qt.io/register to create a free Qt account.
- Download the Qt Online Installer for your OS from here: https://www.qt.io/download-qt-installer-oss
- Sign into the installer.
- Agree to the terms of the (L)GPL 3 license.
- Select whether you would like to send anonymous usage statistics to Qt.
- On the Installation Folder page, leave the default installation path, and select "Custom Installation".

## Customize the installation

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/c6e999e5-cc8a-4dfc-8065-b59139e8c7ae)

Under "Qt", find the latest Qt 6.x release.

Under this release (e.g. Qt 6.5.0), select the target platform:
- On macOS, it is just called "macOS".
- On Windows, it is called "MSVC 2019 64-bit" (for 64-bit x86 CPUs). MinGW has not been tested.

Under this release, select the following additional components:
- Qt 5 Compatibility Module
- Additional Libraries:
  - Qt HTTP Server
  - Qt PDF
- Qt Debug information Files

Under Developer and Designer Tools, select the following components:
- Qt Creator
- Qt Creator CDB Debugger Support (for Windows only)
- Debugging Tools for Windows (for Windows only)
- CMake
- Ninja

Agree to the license and complete the installation.

## Download the source code

You must use git to download the source code for gpt4all:
```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all
```

Note the use of --recurse-submodules, which makes sure the necessary dependencies are downloaded inside the repo. This is why you cannot simply download a zip archive.

Windows users: To install git for Windows, see https://git-scm.com/downloads. Once it is installed, you should be able to shift-right click in any folder, "Open PowerShell window here" (or similar, depending on the version of Windows), and run the above command.

## Open gpt4all-chat in Qt Creator

Open Qt Creator. Navigate to File > Open File or Project, find the "gpt4all-chat" folder inside the freshly cloned repository, and select CMakeLists.txt.

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/3d3e2743-2a1d-43d6-9e55-62f7f4306de7)

## Configure project

You can now expand the "Details" section next to the build kit. It is best to uncheck all but one build configuration, e.g. "Release", which will produce optimized binaries that are not useful for debugging.

Click "Configure Project", and wait for it to complete.

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/44d5aafb-a95d-434b-ba2a-a3138c0e49a0)

## Build project

Now that the project has been configured, click the hammer button on the left sidebar to build the project.

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/43cd7b42-32f0-4efa-9612-d51f85637103)

## Run project

Click the play button on the left sidebar to run the Chat UI.

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/611ea795-bdcd-4feb-a466-eb1c2e936e7e)

## Updating the downloaded source code

You do not need to make a fresh clone of the source code every time. To update it, you may open a terminal/command prompt in the repository, run `git pull`, and then `git submodule update --init --recursive`.
