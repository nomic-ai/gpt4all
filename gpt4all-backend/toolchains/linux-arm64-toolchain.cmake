# Toolchain to crosscompile runtimes for arm64 on jammy x86_64
# You may have to `sudo apt-get install g++-12-aarch64-linux-gnu gcc-12-aarch64-linux-gnu`

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc-12)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++-12)

# Supported backends
set(LLMODEL_CUDA off)
set(LLMODEL_KOMPUTE off)
