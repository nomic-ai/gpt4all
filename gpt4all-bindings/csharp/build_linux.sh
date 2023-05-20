sudo apt-get update
sudo apt-get install -y cmake build-essential
mkdir runtimes
rm -rf runtimes/linux-x64
mkdir runtimes/linux-x64/native
mkdir runtimes/linux-x64/build
cmake -S ../../gpt4all-backend -B runtimes/linux-x64/build
cmake --build runtimes/linux-x64/build --parallel --config Release
cp runtimes/linux-x64/build/libllmodel.so  runtimes/linux-x64/native/libllmodel.so
cp runtimes/linux-x64/build/llama.cpp/libllama.so runtimes/linux-x64/native/libllama.so