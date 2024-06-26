## QA Checklist

1. Ensure you have a fresh install by **backing up** and then deleting the following directories:

    ### Windows
      * Settings directory: ```C:\Users\{username}\AppData\Roaming\nomic.ai```
      * Models directory: ```C:\Users\{username}\AppData\Local\nomic.ai\GPT4All```
    ### Mac
      * Settings directory: ```/Users/{username}/.config/gpt4all.io```
      * Models directory: ```/Users/{username}/Library/Application Support/nomic.ai/GPT4All```
    ### Linux
      * Settings directory: ```/home/{username}/.config/nomic.ai```
      * Models directory: ```/home/{username}/.local/share/nomic.ai/GPT4All```
  
    ^ Note: If you've changed your models directory manually via the settings you need to backup and delete that one

2. Go through every view and ensure that things display correctly and familiarize yourself with the application flow

3. Navigate to the models view and download Llama 3 Instruct

4. Navigate to the models view and search for "TheBloke mistral 7b" and download "TheBloke/Mistral-7B-Instruct-v0.1-GGUF"

5. Navigate to the chat view and open new chats and load these models

6. Chat with the models and exercise them. Rename the chats. Delete chats. Open new chats. Switch models when in chats.

7. Create a new localdocs collection from a directory of .txt or .pdf files on your hard drive

8. Enable the new collection in chats (especially with Llama 3 Instruct) and exercise the localdocs feature

9. Go to the settings view and explore each setting

10. Remove collections in localdocs and re-add them. Rebuild collections

11. Now shut down the app, go back and restore any previous settings directory or model directory you had from a previous install and re-test #1 through #11 :)

12. Try to break the app

### EXTRA CREDIT

1. If you have a openai api key install GPT-4 model and chat with it

2. If you have a nomic api key install the remote nomic embedding model for localdocs (see if you can discover how to do this)

3. If you have a python script that targets openai API then enable server mode and try this

4. Really try and break the app

All feedback is welcome
