# gpt4all-chat

Cross platform Qt based GUI for GPT4All versions with GPT-J as the base
model. NOTE: The model seen in the screenshot is actually a preview of a
new training run for GPT4All based on GPT-J. The GPT4All project is busy
at work getting ready to release this model including installers for all
three major OS's. In the meantime, you can try this UI out with the original
GPT-J model by following build instructions below.

![image](https://user-images.githubusercontent.com/50458173/231464085-da9edff6-a593-410e-8f38-7513f75c8aab.png)

## Install

One click installers for macOS, Linux, and Windows at https://gpt4all.io

## Features

* Cross-platform (Linux, Windows, MacOSX)
* Fast CPU based inference using ggml for GPT-J based models
* The UI is made to look and feel like you've come to expect from a chatty gpt
* Check for updates so you can always stay fresh with latest models
* Easy to install with precompiled binaries available for all three major desktop platforms
* Multi-modal - Ability to load more than one model and switch between them
* Supports both llama.cpp and gptj.cpp style models
* Model downloader in GUI featuring many popular open source models
* Settings dialog to change temp, top_p, top_k, threads, etc
* Copy your conversation to clipboard
* Check for updates to get the very latest GUI

## Feature wishlist

* Multi-chat - a list of current and past chats and the ability to save/delete/export and switch between
* Text to speech - have the AI response with voice
* Speech to text - give the prompt with your voice
* Plugin support for langchain other developer tools
* chat gui headless operation mode
* Advanced settings for changing temperature, topk, etc. (DONE)
* * Improve the accessibility of the installer for screen reader users
* YOUR IDEA HERE

## Building and running

* Follow the visual instructions on the [build_and_run](build_and_run.md) page

## Getting the latest

If you've already checked out the source code and/or built the program make sure when you do a git fetch to get the latest changes and that you also do ```git submodule update --init --recursive``` to update the submodules.

## Manual download of models
* You can find a 'Model Explorer' on the official website where you can manually download models that we support: https://gpt4all.io/index.html

## Terminal Only Interface with no Qt dependency

Check out https://github.com/kuvaus/LlamaGPTJ-chat which is using the llmodel backend so it is compliant with our ecosystem and all models downloaded above should work with it.

## Contributing

* Pull requests welcome. See the feature wish list for ideas :)


## License
The source code of this chat interface is currently under a MIT license. The underlying GPT4All-j model is released under non-restrictive open-source Apache 2 License.

The GPT4All-J license allows for users to use generated outputs as they see fit. Users take responsibility for ensuring their content meets applicable requirements for publication in a given context or region.
