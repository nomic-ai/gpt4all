# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Add Qwen2-1.5B-Instruct to models3.json (by [@ThiloteE](https://github.com/ThiloteE) in [#2759](https://github.com/nomic-ai/gpt4all/pull/2759))

### Changed
- Add missing entries to Italian transltation (by [@Harvester62](https://github.com/Harvester62) in [#2783](https://github.com/nomic-ai/gpt4all/pull/2783))
- Use llama\_kv\_cache ops to shift context faster ([#2781](https://github.com/nomic-ai/gpt4all/pull/2781))
- Don't stop generating at end of context ([#2781](https://github.com/nomic-ai/gpt4all/pull/2781))

### Fixed
- Case-insensitive LocalDocs source icon detection (by [@cosmic-snow](https://github.com/cosmic-snow) in [#2761](https://github.com/nomic-ai/gpt4all/pull/2761))
- Fix comparison of pre- and post-release versions for update check and models3.json ([#2762](https://github.com/nomic-ai/gpt4all/pull/2762), [#2772](https://github.com/nomic-ai/gpt4all/pull/2772))
- Fix several backend issues ([#2778](https://github.com/nomic-ai/gpt4all/pull/2778))
  - Restore leading space removal logic that was incorrectly removed in [#2694](https://github.com/nomic-ai/gpt4all/pull/2694)
  - CUDA: Cherry-pick llama.cpp DMMV cols requirement fix that caused a crash with long conversations since [#2694](https://github.com/nomic-ai/gpt4all/pull/2694)
- Make reverse prompt detection work more reliably and prevent it from breaking output ([#2781](https://github.com/nomic-ai/gpt4all/pull/2781))
- Disallow context shift for chat name and follow-up generation to prevent bugs ([#2781](https://github.com/nomic-ai/gpt4all/pull/2781))

## [3.1.1] - 2024-07-27

### Added
- Add Llama 3.1 8B Instruct to models3.json (by [@3Simplex](https://github.com/3Simplex) in [#2731](https://github.com/nomic-ai/gpt4all/pull/2731) and [#2732](https://github.com/nomic-ai/gpt4all/pull/2732))
- Portuguese (BR) translation (by [thiagojramos](https://github.com/thiagojramos) in [#2733](https://github.com/nomic-ai/gpt4all/pull/2733))
- Support adding arbitrary OpenAI-compatible models by URL (by [@supersonictw](https://github.com/supersonictw) in [#2683](https://github.com/nomic-ai/gpt4all/pull/2683))
- Support Llama 3.1 RoPE scaling ([#2758](https://github.com/nomic-ai/gpt4all/pull/2758))

### Changed
- Add missing entries to Chinese (Simplified) translation (by [wuodoo](https://github.com/wuodoo) in [#2716](https://github.com/nomic-ai/gpt4all/pull/2716) and [#2749](https://github.com/nomic-ai/gpt4all/pull/2749))
- Update translation files and add missing paths to CMakeLists.txt ([#2735](https://github.com/nomic-ai/gpt4all/2735))

## [3.1.0] - 2024-07-24

### Added
- Generate suggested follow-up questions ([#2634](https://github.com/nomic-ai/gpt4all/pull/2634), [#2723](https://github.com/nomic-ai/gpt4all/pull/2723))
  - Also add options for the chat name and follow-up question prompt templates
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
- Server: Reply to wrong GET/POST method with HTTP 405 instead of 404 (by [@cosmic-snow](https://github.com/cosmic-snow) in [#2615](https://github.com/nomic-ai/gpt4all/pull/2615))
- Update theme for menus (by [@3Simplex](https://github.com/3Simplex) in [#2578](https://github.com/nomic-ai/gpt4all/pull/2578))
- Move the "stop" button to the message box ([#2561](https://github.com/nomic-ai/gpt4all/pull/2561))
- Build with CUDA 11.8 for better compatibility ([#2639](https://github.com/nomic-ai/gpt4all/pull/2639))
- Make links in latest news section clickable ([#2643](https://github.com/nomic-ai/gpt4all/pull/2643))
- Support translation of settings choices ([#2667](https://github.com/nomic-ai/gpt4all/pull/2667), [#2690](https://github.com/nomic-ai/gpt4all/pull/2690))
- Improve LocalDocs view's error message (by @cosmic-snow in [#2679](https://github.com/nomic-ai/gpt4all/pull/2679))
- Ignore case of LocalDocs file extensions ([#2642](https://github.com/nomic-ai/gpt4all/pull/2642), [#2684](https://github.com/nomic-ai/gpt4all/pull/2684))
- Update llama.cpp to commit 87e397d00 from July 19th ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694), [#2702](https://github.com/nomic-ai/gpt4all/pull/2702))
  - Add support for GPT-NeoX, Gemma 2, OpenELM, ChatGLM, and Jais architectures (all with Vulkan support)
  - Add support for DeepSeek-V2 architecture (no Vulkan support)
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
- Fix crash/hang when some models stop generating, by showing special tokens ([#2701](https://github.com/nomic-ai/gpt4all/pull/2701))

[Unreleased]: https://github.com/nomic-ai/gpt4all/compare/v3.1.1...HEAD
[3.1.1]: https://github.com/nomic-ai/gpt4all/compare/v3.1.0...v3.1.1
[3.1.0]: https://github.com/nomic-ai/gpt4all/compare/v3.0.0...v3.1.0
