#!/bin/sh
mkdir -p runtimes
rm -rf runtimes/linux-x64
mkdir -p runtimes/linux-x64/native
mkdir runtimes/linux-x64/build
cmake -S ../../gpt4all-backend -B runtimes/linux-x64/build
cmake --build runtimes/linux-x64/build --parallel --config Release
cp runtimes/linux-x64/build/libllmodel.so  runtimes/linux-x64/native/libllmodel.so
cp runtimes/linux-x64/build/libgptj*.so  runtimes/linux-x64/native/
cp runtimes/linux-x64/build/libllama*.so  runtimes/linux-x64/native/
