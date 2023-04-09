# gpt4all-chat

Cross platform Qt based GUI for GPT4All versions with GPT-J as the base
model. NOTE: The model seen in the screenshot is actually the original
GPT-J model. The GPT4All project is busy at work training a new version
with GPT-J as the base, but it isn't ready yet. In the meantime, you can
try this UI out with the original GPT-J model. Full instructions for how
are included below.

![image](https://user-images.githubusercontent.com/50458173/230756282-31b43d63-3c98-4f5d-b2dc-8c1ec2e0c8a9.png)

## Features

* Cross-platform (Linux, Windows, MacOSX, iOS, Android, Embedded Linux, QNX)
* Fast CPU based inference using ggml for GPT-J based models
* The UI is made to look and feel like you've come to expect from a chatty gpt
* Easy to install... The plan is to create precompiled binaries for major platforms with easy installer including model

## Building and running

* Install Qt 6.x for your platform https://doc.qt.io/qt-6/get-and-install-qt.html
* Install cmake for your platform https://cmake.org/install/
* Download https://huggingface.co/EleutherAI/gpt-j-6b
* Clone this repo and build
```
git clone --recurse-submodules https://github.com/manyoso/gpt4all-chat.git
cd gpt4all-chat
mkdir build
cd build
cmake ..
cmake --build . --parallel
python3 ../ggml/examples/gpt-j/convert-h5-to-ggml.py /path/to/your/local/copy/of/EleutherAI/gpt-j-6B 0
./bin/gpt-j-quantize /path/to/your/local/copy/of/EleutherAI/gpt-j-6B/ggml-model-f32.bin ./ggml-model-q4_0.bin 2
./chat
```

## Contributing

* Pull requests welcome :)

