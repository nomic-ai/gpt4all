# Contributing Foreign Language Translations of GPT4All

## Overview

To contribute foreign language translations to the GPT4All project will require
installation of a graphical tool called Qt Linguist. This tool can be obtained by
installing a subset of Qt. You'll also need to clone this github repository locally
on your filesystem.

Once this tool is installed you'll be able to use it to load specific translation
files found in the gpt4all github repository and add your foreign language translations.
Once you've done this you can contribute back those translations by opening a pull
request on Github or by sharing it with one of the administrators on GPT4All [discord.](https://discord.gg/4M2QFmTt2k)


## Download Qt Linguist

- Go to https://login.qt.io/register to create a free Qt account.
- Download the Qt Online Installer for your OS from here: https://www.qt.io/download-qt-installer-oss
- Sign into the installer.
- Agree to the terms of the (L)GPL 3 license.
- Select whether you would like to send anonymous usage statistics to Qt.
- On the Installation Folder page, leave the default installation path, and select "Custom Installation".

![image](https://github.com/nomic-ai/gpt4all/assets/10168/85234549-1ea7-43c9-87d1-1e4f0fb93d82)

- Under "Qt", select the latest Qt 6.x release as well as Developer and Designer Tools
- NOTE: This will install much more than the Qt Linguist tool and you can deselect portions, but to be
  safe I've included the easiest steps that will also enable you to build GPT4All from source if you wish.

## Open Qt Linguist

After installation you should be able to find the Qt Linguist application in the following locations:

- Windows `C:\Qt\6.7.2\msvc2019_64\bin\linguist.exe`
- macOS `/Users/username/Qt/6.7.2/macos/bin/Linguist.app`
- Linux `/home/username/Qt/6.7.2/gcc_64/bin/linguist`

![Peek 2024-07-11 10-26](https://github.com/nomic-ai/gpt4all/assets/10168/957de16f-4e23-4d90-9d20-9089d2028aa8)

## After you've opened Qt Linguist

- Navigate to the translation file you're interested in contributing to. This file will be located
  in the gpt4all `translations` directory found on your local filesystem after you've cloned the
  gpt4all github repository. It is this folder [gpt4all/gpt4all-chat/translations](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-chat/translations)
  located on your local filesystem after cloning the repository.
- If the file does not exist yet for the language you are interested in, then just copy the english one
  to a new file with appropriate name and edit that.

## How to see your translations in the app as you develop them
![Peek 2024-07-12 14-22](https://github.com/user-attachments/assets/6ff00338-5b49-4f97-a0d4-de96f3991469)
- In the same folder that where your models are stored you can add translation files (.ts) and compile them
  using the command `/path/to/Qt/6.7.2/gcc_64/bin/lrelease gpt4all_{lang}.ts`
- This should produce a file named `gpt4all_{lang}.qm` in the same folder. Restart GPT4All and you should
  now be able to see your language in the settings combobox.

  
## Information on how to use Qt Linguist

- [Manual for translators](https://doc.qt.io/qt-6/linguist-translators.html)
- [Video explaining how translators use Qt Linguist](https://youtu.be/xNIz78IPBu0?t=351)

## Once you've edited the translations save the file
- Open a [pull request](https://github.com/nomic-ai/gpt4all/pulls) for your changes.
- Alternatively, you may share your translation file with one of the administrators on GPT4All [discord.](https://discord.gg/4M2QFmTt2k)


# Thank you!
