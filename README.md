# gpt4all-chat

Cross platform Qt based GUI for GPT4All versions with GPT-J as the base
model. NOTE: The model seen in the screenshot is actually a preview of a
new training run for GPT4All based on GPT-J. The GPT4All project is busy
at work getting ready to release this model including installers for all
three major OS's. In the meantime, you can try this UI out with the original
GPT-J model by following build instructions below.

![image](https://user-images.githubusercontent.com/50458173/231464085-da9edff6-a593-410e-8f38-7513f75c8aab.png)

## Features

* Cross-platform (Linux, Windows, MacOSX)
* Fast CPU based inference using ggml for GPT-J based models
* The UI is made to look and feel like you've come to expect from a chatty gpt
* Easy to install... The plan is to create precompiled binaries for major platforms with easy installers which include the model
* WORK IN PROGRESS!!

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

