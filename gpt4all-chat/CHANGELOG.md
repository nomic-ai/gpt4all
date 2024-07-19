# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Generate suggested follow-up questions ([#2634](https://github.com/nomic-ai/gpt4all/pull/2634))
- Scaffolding for translations ([#2612](https://github.com/nomic-ai/gpt4all/pull/2612))
- Spanish (MX) translation (by [@jstayco](https://github.com/jstayco) in [#2654](https://github.com/nomic-ai/gpt4all/pull/2654))
- Chinese (Simplified) translation by mikage ([#2657](https://github.com/nomic-ai/gpt4all/pull/2657))
- Dynamic changes of language and locale at runtime ([#2659](https://github.com/nomic-ai/gpt4all/pull/2659), [#2677](https://github.com/nomic-ai/gpt4all/pull/2677))
- Romanian translation by [@SINAPSA\_IC](https://github.com/SINAPSA_IC) ([#2662](https://github.com/nomic-ai/gpt4all/pull/2662))
- Chinese (Traditional) translation (by [@supersonictw](https://github.com/supersonictw) in [#2661](https://github.com/nomic-ai/gpt4all/pull/2661))
- Italian translation (by [@Harvester62](https://github.com/Harvester62) in [#2700](https://github.com/nomic-ai/gpt4all/pull/2700))

### Changed
- Customize combo boxes and context menus to fit the new style ([#2535](https://github.com/nomic-ai/gpt4all/pull/2535))
- Improve view bar scaling and Model Settings layout ([#2520](https://github.com/nomic-ai/gpt4all/pull/2520)
- Make the logo spin while the model is generating ([#2557](https://github.com/nomic-ai/gpt4all/pull/2557))
- Server: Reply to wrong GET/POST method with HTTP 405 instead of 404 (by @cosmic-snow in [#2615](https://github.com/nomic-ai/gpt4all/pull/2615))
- Update theme for menus (by @3Simplex in [#2578](https://github.com/nomic-ai/gpt4all/pull/2578))
- Move the "stop" button to the message box ([#2561](https://github.com/nomic-ai/gpt4all/pull/2561))
- Build with CUDA 11.8 for better compatibility ([#2639](https://github.com/nomic-ai/gpt4all/pull/2639))
- Make links in latest news section clickable ([#2643](https://github.com/nomic-ai/gpt4all/pull/2643))
- Support translation of settings choices ([#2667](https://github.com/nomic-ai/gpt4all/pull/2667), [#2690](https://github.com/nomic-ai/gpt4all/pull/2690))
- Improve LocalDocs view's error message (by @cosmic-snow in [#2679](https://github.com/nomic-ai/gpt4all/pull/2679))
- Ignore case of LocalDocs file extensions ([#2642](https://github.com/nomic-ai/gpt4all/pull/2642), [#2684](https://github.com/nomic-ai/gpt4all/pull/2684))
- Update llama.cpp to commit 87e397d00 from July 19th ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694))
  - Add support for GPT-NeoX, Gemma 2, OpenELM, ChatGLM, and Jais architectures (all with Vulkan support)
  - Enable Vulkan support for StarCoder2, XVERSE, Command R, and OLMo
- Show scrollbar in chat collections list as needed (by [@cosmic-snow](https://github.com/cosmic-snow) in [#2691](https://github.com/nomic-ai/gpt4all/pull/2691))

### Removed
- Remove support for GPT-J models ([#2676](https://github.com/nomic-ai/gpt4all/pull/2676), [#2693](https://github.com/nomic-ai/gpt4all/pull/2693))

### Fixed
- Fix placement of thumbs-down and datalake opt-in dialogs ([#2540](https://github.com/nomic-ai/gpt4all/pull/2540))
- Select the correct folder with the Linux fallback folder dialog ([#2541](https://github.com/nomic-ai/gpt4all/pull/2541))
- Fix clone button sometimes producing blank model info ([#2545](https://github.com/nomic-ai/gpt4all/pull/2545))
- Fix jerky chat view scrolling ([#2555](https://github.com/nomic-ai/gpt4all/pull/2555))
- Fix "reload" showing for chats with missing models ([#2520](https://github.com/nomic-ai/gpt4all/pull/2520)
- Fix property binding loop warning ([#2601](https://github.com/nomic-ai/gpt4all/pull/2601))
- Fix UI hang with certain chat view content ([#2543](https://github.com/nomic-ai/gpt4all/pull/2543))
- Fix crash when Kompute falls back to CPU ([#2640](https://github.com/nomic-ai/gpt4all/pull/2640))
- Fix several Vulkan resource management issues ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694))

[Unreleased]: https://github.com/nomic-ai/gpt4all/compare/v3.0.0...HEAD
