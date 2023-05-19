# Install Qt 6.x and setup/build gpt4all-chat from source

Depending upon your operating system, there are many ways that Qt is distributed. 
Here is the recommended method for getting the Qt dependency installed to setup and build 
gpt4all-chat from source.

## Create a [Qt account](https://login.qt.io/register) 

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/d1e44cab-4245-4144-a91c-7b02267df2b2)

## Go to the Qt open source [download page](https://www.qt.io/download-qt-installer-oss)

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/d68f5f45-cca3-4fe9-acf4-cabdcb95f669)

## Start the installer and sign in

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/899b1422-51ae-4bb5-acc9-b9027a8e9b19)

## After some screens about license, select custom

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/2290031a-fdb0-4f47-a7f1-d77ad5451068)

## Select the following

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/c6e999e5-cc8a-4dfc-8065-b59139e8c7ae)

NOTE: This is for macOS. For Linux it is similar, but you need ming64 for Windows, not the MSVC install

## Open up QtCreator

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/a34978f4-a220-459c-af66-e901d7ccd7bb)

## Clone the git repo for gpt4all-chat

```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all
```

## Open the gpt4all-chat project in QtCreator

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/3d3e2743-2a1d-43d6-9e55-62f7f4306de7)

NOTE: File->Open File or Project and navigate to the gpt4all-chat repo and choose the CMakeLists.txt

## Configure project

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/44d5aafb-a95d-434b-ba2a-a3138c0e49a0)

## Build project

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/43cd7b42-32f0-4efa-9612-d51f85637103)

## Run project

![image](https://github.com/nomic-ai/gpt4all-chat/assets/10168/611ea795-bdcd-4feb-a466-eb1c2e936e7e)


