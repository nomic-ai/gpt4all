# Using GPT4All to Privately Chat with your Obsidian Vault

Obsidian for Desktop is a powerful management and note-taking software designed to create and organize markdown notes. This tutorial allows you to sync and access your Obsidian note files directly on your computer. By connecting it to LocalDocs, you can integrate these files into your LLM chats for private access and enhanced context.

## Download Obsidian for Desktop

!!! note "Download Obsidian for Desktop"

      1. **Download Obsidian for Desktop**:
         - Visit the [Obsidian website](https://obsidian.md) and create an account account.
         - Click the Download button in the center of the homepage
         - For more help with installing Obsidian see [Getting Started with Obsidian](https://help.obsidian.md/Getting+started/Download+and+install+Obsidian)
      
      2. **Set Up Obsidian**:
         - Launch Obsidian from your Applications folder (macOS), Start menu (Windows), or equivalent location (Linux).
         - On the welcome screen, you can either create a new vault (a collection of notes) or open an existing one.
         - To create a new vault, click Create a new vault, name your vault, choose a location on your computer, and click Create.
   
   
      3. **Sign in and Sync**:
            - Once installed, you can start adding and organizing notes.
            - Choose the folders you want to sync to your computer.
   


## Connect Obsidian to LocalDocs

!!! note "Connect Obsidian to LocalDocs"

      1. **Open LocalDocs**:
         - Navigate to the LocalDocs feature within GPT4All.

         <table>
            <tr>
               <td>
                  <!-- Screenshot of LocalDocs interface -->
                  <img width="1348" alt="LocalDocs interface" src="https://github.com/nomic-ai/gpt4all/assets/132290469/d8fb2d79-2063-45d4-bcce-7299fb75b144">
               </td>
            </tr>
         </table>
   
      2. **Add Collection**:
         - Click on **+ Add Collection** to begin linking your Obsidian Vault.
      
         <table>
            <tr>
               <td>
                  <!-- Screenshot of adding collection in LocalDocs -->
                  <img width="1348" alt="Screenshot of adding collection" src="https://raw.githubusercontent.com/nomic-ai/gpt4all/124ef867a9d9afd9e14d3858cd77bce858f79773/gpt4all-bindings/python/docs/assets/obsidian_adding_collection.png">
               </td>
            </tr>
         </table>
   
         - Name your collection
   
   
      3. **Create Collection**:
         - Click **Create Collection** to initiate the embedding process. Progress will be displayed within the LocalDocs interface.
   
      4. **Access Files in Chats**:
         - Load a model to chat with your files (Llama 3 Instruct is the fastest)
         - In your chat, open 'LocalDocs' with the button in the top-right corner to provide context from your synced Obsidian notes.
      
         <table>
            <tr>
               <td>
                  <!-- Screenshot of accessing LocalDocs in chats -->
                  <img width="1447" alt="Accessing LocalDocs in chats" src="https://raw.githubusercontent.com/nomic-ai/gpt4all/124ef867a9d9afd9e14d3858cd77bce858f79773/gpt4all-bindings/python/docs/assets/obsidian_docs.png">
               </td>
            </tr>
         </table>
   
      5. **Interact With Your Notes:**
         - Use the model to interact with your files
         <table>
            <tr>
               <td>
                  <!-- Screenshot of interacting sources -->
                  <img width="662" alt="osbsidian user interaction" src="https://raw.githubusercontent.com/nomic-ai/gpt4all/124ef867a9d9afd9e14d3858cd77bce858f79773/gpt4all-bindings/python/docs/assets/osbsidian_user_interaction.png">
               </td>
            </tr>
         </table>
         <table>
            <tr>
               <td>
                  <!-- Screenshot of viewing sources -->
                  <img width="662" alt="osbsidian GPT4ALL response" src="https://raw.githubusercontent.com/nomic-ai/gpt4all/124ef867a9d9afd9e14d3858cd77bce858f79773/gpt4all-bindings/python/docs/assets/obsidian_response.png">
               </td>
            </tr>
         </table>
   
      6. **View Referenced Files**:
         - Click on **Sources** below LLM responses to see which Obsidian Notes were referenced.
      
         <table>
            <tr>
               <td>
                  <!-- Referenced Files  -->
                  <img width="643" alt="Referenced Files" src="https://raw.githubusercontent.com/nomic-ai/gpt4all/124ef867a9d9afd9e14d3858cd77bce858f79773/gpt4all-bindings/python/docs/assets/obsidian_sources.png">
               </td>
            </tr>
         </table>

## How It Works

Obsidian for Desktop syncs your Obsidian notes to your computer, while LocalDocs integrates these files into your LLM chats using embedding models. These models find semantically similar snippets from your files to enhance the context of your interactions.

To learn more about embedding models and explore further, refer to the [Nomic Python SDK documentation](https://docs.nomic.ai/atlas/capabilities/embeddings).

