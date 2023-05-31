mkdir -p runtimes
rm -rf runtimes/osx
mkdir -p runtimes/osx/native
mkdir runtimes/osx/build
cmake -S ../../gpt4all-backend -B runtimes/osx/build -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build runtimes/osx/build --parallel --config Release
cp runtimes/osx/build/libllmodel.dylib  runtimes/osx/native/libllmodel.dylib
cp runtimes/osx/build/llama.cpp/libllama.dylib runtimes/osx/native/libllama.dylib
