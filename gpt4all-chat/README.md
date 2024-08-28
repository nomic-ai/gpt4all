# gpt4all-chat

Cross platform Qt based GUI for GPT4All versions with GPT-J as the base
model. NOTE: The model seen in the screenshot is actually a preview of a
new training run for GPT4All based on GPT-J. The GPT4All project is busy
at work getting ready to release this model including installers for all
three major OS's. In the meantime, you can try this UI out with the original
GPT-J model by following build instructions below.

![image](https://user-images.githubusercontent.com/50458173/231464085-da9edff6-a593-410e-8f38-7513f75c8aab.png)

## Install

One click installers for macOS, Linux, and Windows at https://www.nomic.ai/gpt4all

## Features

* Cross-platform (Linux, Windows, MacOSX)
* The UI is made to look and feel like you've come to expect from a chatty gpt
* Check for updates so you can always stay fresh with latest models
* Easy to install with precompiled binaries available for all three major desktop platforms
* Multi-modal - Ability to load more than one model and switch between them
* Multi-chat - a list of current and past chats and the ability to save/delete/export and switch between
* Supports models that are supported by llama.cpp
* Model downloader in GUI featuring many popular open source models
* Settings dialog to change temp, top_p, min_p, top_k, threads, etc
* Copy your conversation to clipboard
* RAG via LocalDocs feature
* Check for updates to get the very latest GUI

## Building and running

* Follow the visual instructions on the [build_and_run](build_and_run.md) page

## Getting the latest

If you've already checked out the source code and/or built the program make sure when you do a git fetch to get the latest changes and that you also do `git submodule update --init --recursive` to update the submodules. (If you ever run into trouble, deinitializing via `git submodule deinit -f .` and then initializing again via `git submodule update --init --recursive` fixes most issues)

## Contributing

* Pull requests welcome. See the feature wish list for ideas :)


## License
The source code of this chat interface is currently under a MIT license.
