# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Add bm25 hybrid search to localdocs ([#2969](https://github.com/nomic-ai/gpt4all/pull/2969))

### Changed
- Rebase llama.cpp on latest upstream as of September 26th ([#2998](https://github.com/nomic-ai/gpt4all/pull/2998))

### Fixed
- Fix a crash when attempting to continue a chat loaded from disk ([#2995](https://github.com/nomic-ai/gpt4all/pull/2995))
- Fix the local server rejecting min\_p/top\_p less than 1 ([#2996](https://github.com/nomic-ai/gpt4all/pull/2996))

## [3.3.1] - 2024-09-27 ([v3.3.y](https://github.com/nomic-ai/gpt4all/tree/v3.3.y))

### Fixed
- Fix a crash when attempting to continue a chat loaded from disk ([#2995](https://github.com/nomic-ai/gpt4all/pull/2995))
- Fix the local server rejecting min\_p/top\_p less than 1 ([#2996](https://github.com/nomic-ai/gpt4all/pull/2996))

## [3.3.0] - 2024-09-20

### Added
- Use greedy sampling when temperature is set to zero ([#2854](https://github.com/nomic-ai/gpt4all/pull/2854))
- Use configured system prompt in server mode and ignore system messages ([#2921](https://github.com/nomic-ai/gpt4all/pull/2921), [#2924](https://github.com/nomic-ai/gpt4all/pull/2924))
- Add more system information to anonymous usage stats ([#2939](https://github.com/nomic-ai/gpt4all/pull/2939))
- Check for unsupported Ubuntu and macOS versions at install time ([#2940](https://github.com/nomic-ai/gpt4all/pull/2940))

### Changed
- The offline update button now directs users to the offline installer releases page. (by [@3Simplex](https://github.com/3Simplex) in [#2888](https://github.com/nomic-ai/gpt4all/pull/2888))
- Change the website link on the home page to point to the new URL ([#2915](https://github.com/nomic-ai/gpt4all/pull/2915))
- Smaller default window size, dynamic minimum size, and scaling tweaks ([#2904](https://github.com/nomic-ai/gpt4all/pull/2904))
- Only allow a single instance of program to be run at a time ([#2923](https://github.com/nomic-ai/gpt4all/pull/2923]))

### Fixed
- Bring back "Auto" option for Embeddings Device as "Application default," which went missing in v3.1.0 ([#2873](https://github.com/nomic-ai/gpt4all/pull/2873))
- Correct a few strings in the Italian translation (by [@Harvester62](https://github.com/Harvester62) in [#2872](https://github.com/nomic-ai/gpt4all/pull/2872) and [#2909](https://github.com/nomic-ai/gpt4all/pull/2909))
- Correct typos in Traditional Chinese translation (by [@supersonictw](https://github.com/supersonictw) in [#2852](https://github.com/nomic-ai/gpt4all/pull/2852))
- Set the window icon on Linux ([#2880](https://github.com/nomic-ai/gpt4all/pull/2880))
- Corrections to the Romanian translation (by [@SINAPSA-IC](https://github.com/SINAPSA-IC) in [#2890](https://github.com/nomic-ai/gpt4all/pull/2890))
- Fix singular/plural forms of LocalDocs "x Sources" (by [@cosmic-snow](https://github.com/cosmic-snow) in [#2885](https://github.com/nomic-ai/gpt4all/pull/2885))
- Fix a typo in Model Settings (by [@3Simplex](https://github.com/3Simplex) in [#2916](https://github.com/nomic-ai/gpt4all/pull/2916))
- Fix the antenna icon tooltip when using the local server ([#2922](https://github.com/nomic-ai/gpt4all/pull/2922))
- Fix a few issues with locating files and handling errors when loading remote models on startup ([#2875](https://github.com/nomic-ai/gpt4all/pull/2875))
- Significantly improve API server request parsing and response correctness ([#2929](https://github.com/nomic-ai/gpt4all/pull/2929))
- Remove unnecessary dependency on Qt WaylandCompositor module ([#2949](https://github.com/nomic-ai/gpt4all/pull/2949))
- Update translations ([#2970](https://github.com/nomic-ai/gpt4all/pull/2970))
- Fix macOS installer and remove extra installed copy of Nomic Embed ([#2973](https://github.com/nomic-ai/gpt4all/pull/2973))

## [3.2.1] - 2024-08-13

### Fixed
- Do not initialize Vulkan driver when only using CPU ([#2843](https://github.com/nomic-ai/gpt4all/pull/2843))
- Fix a potential crash on exit when using only CPU on Linux with NVIDIA (does not affect X11) ([#2843](https://github.com/nomic-ai/gpt4all/pull/2843))
- Fix default CUDA architecture list after [#2802](https://github.com/nomic-ai/gpt4all/pull/2802) ([#2855](https://github.com/nomic-ai/gpt4all/pull/2855))

## [3.2.0] - 2024-08-12

### Added
- Add Qwen2-1.5B-Instruct to models3.json (by [@ThiloteE](https://github.com/ThiloteE) in [#2759](https://github.com/nomic-ai/gpt4all/pull/2759))
- Enable translation feature for seven languages: English, Spanish, Italian, Portuguese, Chinese Simplified, Chinese Traditional, Romanian ([#2830](https://github.com/nomic-ai/gpt4all/pull/2830))

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
- Explicitly target macOS 12.6 in CI to fix Metal compatibility on older macOS ([#2846](https://github.com/nomic-ai/gpt4all/pull/2846))

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

[Unreleased]: https://github.com/nomic-ai/gpt4all/compare/v3.3.0...HEAD
[3.3.1]: https://github.com/nomic-ai/gpt4all/compare/v3.3.0...v3.3.1
[3.3.0]: https://github.com/nomic-ai/gpt4all/compare/v3.2.1...v3.3.0
[3.2.1]: https://github.com/nomic-ai/gpt4all/compare/v3.2.0...v3.2.1
[3.2.0]: https://github.com/nomic-ai/gpt4all/compare/v3.1.1...v3.2.0
[3.1.1]: https://github.com/nomic-ai/gpt4all/compare/v3.1.0...v3.1.1
[3.1.0]: https://github.com/nomic-ai/gpt4all/compare/v3.0.0...v3.1.0
