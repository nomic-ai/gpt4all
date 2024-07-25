
# GPT4All 2024 Roadmap
To contribute to the development of any of the below roadmap items, make or find the corresponding issue and cross-reference the [in-progress task](https://github.com/orgs/nomic-ai/projects/2/views/1).

Each item should have an issue link below.

- Chat UI Language Localization (localize UI into the native languages of users)
    - [ ] Chinese
    - [ ] German
    - [ ] French
    - [x] Portuguese
    - [ ] Your native language here. 
- UI Redesign: an internal effort at Nomic to improve the UI/UX of gpt4all for all users.
    - [x] Design new user interface and gather community feedback
    - [x] Implement the new user interface and experience.
- Installer and Update Improvements
    - [ ] Seamless native installation and update process on OSX
    - [ ] Seamless native installation and update process on Windows
    - [ ] Seamless native installation and update process on Linux
- Model discoverability improvements:
    - [x] Support huggingface model discoverability
    - [x] Support Nomic hosted model discoverability
- LocalDocs (towards a local perplexity)
    - Multilingual LocalDocs Support
        - [ ] Create a multilingual experience
        - [ ] Incorporate a multilingual embedding model
        - [ ] Specify a preferred multilingual LLM for localdocs
    - Improved RAG techniques
        - [ ] Query augmentation and re-writing
        - [ ] Improved chunking and text extraction from arbitrary modalities
            - [ ] Custom PDF extractor past the QT default (charts, tables, text)
        - [ ] Faster indexing and local exact search with v1.5 hamming embeddings and reranking (skip ANN index construction!)
    - Support queries like 'summarize X document'
    - Multimodal LocalDocs support with Nomic Embed
    - Nomic Dataset Integration with real-time LocalDocs
        - [ ] Include an option to allow the export of private LocalDocs collections to Nomic Atlas for debugging data/chat quality
        - [ ] Allow optional sharing of LocalDocs collections between users.
        - [ ] Allow the import of a LocalDocs collection from an Atlas Datasets
            - Chat with live version of Wikipedia, Chat with Pubmed, chat with the latest snapshot of world news.
- First class Multilingual LLM Support
    - [ ] Recommend and set a default LLM for German
    - [ ] Recommend and set a default LLM for English
    - [ ] Recommend and set a default LLM for Chinese
    - [ ] Recommend and set a default LLM for Spanish

- Server Mode improvements
    - Improved UI and new requested features:
        - [ ] Fix outstanding bugs and feature requests around networking configurations.
        - [ ] Support Nomic Embed inferencing
        - [ ] First class documentation
        - [ ] Improving developer use and quality of server mode (e.g. support larger batches)