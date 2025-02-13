# Using GPT4All to Privately Chat with your OneDrive Data

Local and Private AI Chat with your OneDrive Data

OneDrive for Desktop allows you to sync and access your OneDrive files directly on your computer. By connecting your synced directory to LocalDocs, you can start using GPT4All to privately chat with data stored in your OneDrive.

## Download OneDrive for Desktop

!!! note "Download OneDrive for Desktop"

    1. **Download OneDrive for Desktop**:
    - Visit [Microsoft OneDrive](https://www.microsoft.com/en-us/microsoft-365/onedrive/download).
    - Press 'download' for your respective device type.
    - Download the OneDrive for Desktop application.
    
    2. **Install OneDrive for Desktop**
    - Run the installer file you downloaded.
    - Follow the prompts to complete the installation process.
    
    3. **Sign in and Sync**
    - Once installed, sign in to OneDrive for Desktop with your Microsoft account credentials.
    - Choose the folders you want to sync to your computer.

## Connect OneDrive to LocalDocs

!!! note "Connect OneDrive to LocalDocs"

    1. **Install GPT4All and Open LocalDocs**:
    
        - Go to [nomic.ai/gpt4all](https://nomic.ai/gpt4all) to install GPT4All for your operating system.
        
        - Navigate to the LocalDocs feature within GPT4All to configure it to use your synced OneDrive directory.

        <table>
        <tr>
            <td>
                <!-- Placeholder for screenshot of LocalDocs interface -->
                <img width="1348" alt="Screenshot 2024-07-10 at 10 55 41 AM" src="https://github.com/nomic-ai/gpt4all/assets/132290469/54254bc0-d9a0-40c4-9fd1-5059abaad583">
            </td>
        </tr>
        </table>

    2. **Add Collection**:
    
        - Click on **+ Add Collection** to begin linking your OneDrive folders.

        <table>
        <tr>
            <td>
                <!-- Placeholder for screenshot of adding collection in LocalDocs -->
               <img width="1348" alt="Screenshot 2024-07-10 at 10 56 29 AM" src="https://github.com/nomic-ai/gpt4all/assets/132290469/7f12969a-753a-4757-bb9e-9b607cf315ca">
            </td>
        </tr>
        </table>

        - Name the Collection and specify the OneDrive folder path.

    3. **Create Collection**:
    
        - Click **Create Collection** to initiate the embedding process. Progress will be displayed within the LocalDocs interface.

    4. **Access Files in Chats**:
    
        - Load a model within GPT4All to chat with your files.
        
        - In your chat, open 'LocalDocs' using the button in the top-right corner to provide context from your synced OneDrive files.

        <table>
        <tr>
            <td>
                <!-- Placeholder for screenshot of accessing LocalDocs in chats -->
                <img width="1447" alt="Screenshot 2024-07-10 at 10 58 55 AM" src="https://github.com/nomic-ai/gpt4all/assets/132290469/b5a67fe6-0d6a-42ae-b3b8-cc0f91cbf5b1">
            </td>
        </tr>
        </table>

    5. **Interact With Your OneDrive**:
    
        - Use the model to interact with your files directly from OneDrive.
        
        <table>
        <tr>
            <td>
                <!-- Placeholder for screenshot of interacting with sources -->
                <img width="662" alt="Screenshot 2024-07-10 at 11 04 55 AM" src="https://github.com/nomic-ai/gpt4all/assets/132290469/2c9815b8-3d1c-4179-bf76-3ddbafb193bf">
            </td>
        </tr>
        </table>
        
        <table>
        <tr>
            <td>
                <img width="662" alt="Screenshot 2024-07-11 at 11 21 46 AM" src="https://github.com/nomic-ai/gpt4all/assets/132290469/ce8be292-b025-415a-bd54-f11868e0cd0a">
            </td>
        </tr>
        </table>

    6. **View Referenced Files**:
    
        - Click on **Sources** below responses to see which OneDrive files were referenced.

        <table>
        <tr>
            <td>
              <img width="643" alt="Screenshot 2024-07-11 at 11 22 49 AM" src="https://github.com/nomic-ai/gpt4all/assets/132290469/6fe3f10d-2791-4153-88a7-2198ab3ac945">
            </td>
        </tr>
        </table>

## How It Works

OneDrive for Desktop syncs your OneDrive files to your computer, while LocalDocs maintains a database of these synced files for use by your local GPT4All model. As your OneDrive updates, LocalDocs will automatically detect file changes and stay up to date. LocalDocs leverages [Nomic Embedding](https://docs.nomic.ai/atlas/capabilities/embeddings) models to find semantically similar snippets from your files, enhancing the context of your interactions.
