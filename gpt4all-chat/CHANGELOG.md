# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Fixed
- Fix the timeout error in code interpreter ([#3369](https://github.com/nomic-ai/gpt4all/pull/3369))
- Fix code interpreter console.log not accepting multiple arguments ([#3371](https://github.com/nomic-ai/gpt4all/pull/3371))
- Remove 'X is defined' checks from templates as they work incorrectly with Jinja2Cpp ([#3372](https://github.com/nomic-ai/gpt4all/pull/3372))
- Jinja2Cpp: Add 'if' requirement for 'else' parsing to fix crash ([#3373](https://github.com/nomic-ai/gpt4all/pull/3373))
- Save chats on quit, even if the window isn't closed first ([#3387](https://github.com/nomic-ai/gpt4all/pull/3387))

## [3.6.1] - 2024-12-20

### Fixed
- Fix the stop generation button no longer working in v3.6.0 ([#3336](https://github.com/nomic-ai/gpt4all/pull/3336))
- Fix the copy entire conversation button no longer working in v3.6.0 ([#3336](https://github.com/nomic-ai/gpt4all/pull/3336))

## [3.6.0] - 2024-12-19

### Added
- Automatically substitute chat templates that are not compatible with Jinja2Cpp in GGUFs ([#3327](https://github.com/nomic-ai/gpt4all/pull/3327))
- Built-in javascript code interpreter tool plus model ([#3173](https://github.com/nomic-ai/gpt4all/pull/3173))

### Fixed
- Fix remote model template to allow for XML in messages ([#3318](https://github.com/nomic-ai/gpt4all/pull/3318))
- Fix Jinja2Cpp bug that broke system message detection in chat templates ([#3325](https://github.com/nomic-ai/gpt4all/pull/3325))
- Fix LocalDocs sources displaying in unconsolidated form after v3.5.0 ([#3328](https://github.com/nomic-ai/gpt4all/pull/3328))

## [3.5.3] - 2024-12-16

### Fixed
- Fix LocalDocs not using information from sources in v3.5.2 ([#3302](https://github.com/nomic-ai/gpt4all/pull/3302))

## [3.5.2] - 2024-12-13

### Added
- Create separate download pages for built-in and HuggingFace models ([#3269](https://github.com/nomic-ai/gpt4all/pull/3269))

### Fixed
- Fix API server ignoring assistant messages in history after v3.5.0 ([#3256](https://github.com/nomic-ai/gpt4all/pull/3256))
- Fix API server replying with incorrect token counts and stop reason after v3.5.0 ([#3256](https://github.com/nomic-ai/gpt4all/pull/3256))
- Fix API server remembering previous, unrelated conversations after v3.5.0 ([#3256](https://github.com/nomic-ai/gpt4all/pull/3256))
- Fix mishandling of default chat template and system message of cloned models in v3.5.0 ([#3262](https://github.com/nomic-ai/gpt4all/pull/3262))
- Fix untranslated text on the startup dialog ([#3293](https://github.com/nomic-ai/gpt4all/pull/3293))

## [3.5.1] - 2024-12-10

### Fixed
- Fix an incorrect value for currentResponse ([#3245](https://github.com/nomic-ai/gpt4all/pull/3245))
- Fix the default model button so it works again after 3.5.0 ([#3246](https://github.com/nomic-ai/gpt4all/pull/3246))
- Fix chat templates for Nous Hermes 2 Mistral, Mistral OpenOrca, Qwen 2, and remote models ([#3250](https://github.com/nomic-ai/gpt4all/pull/3250))
- Fix chat templates for Llama 3.2 models ([#3251](https://github.com/nomic-ai/gpt4all/pull/3251))

## [3.5.0] - 2024-12-09

### Changed
- Update Italian translation (by [@Harvester62](https://github.com/Harvester62) in [#3236](https://github.com/nomic-ai/gpt4all/pull/3236))
- Update Romanian translation (by [@SINAPSA-IC](https://github.com/SINAPSA-IC) in [#3232](https://github.com/nomic-ai/gpt4all/pull/3232))

### Fixed
- Fix a few more problems with the Jinja changes ([#3239](https://github.com/nomic-ai/gpt4all/pull/3239))

## [3.5.0-rc2] - 2024-12-06

### Changed
- Fade messages out with an animation when they are removed from the chat view ([#3227](https://github.com/nomic-ai/gpt4all/pull/3227))
- Tweak wording of edit/redo confirmation dialogs ([#3228](https://github.com/nomic-ai/gpt4all/pull/3228))
- Make edit/redo buttons disabled instead of invisible when they are temporarily unavailable ([#3228](https://github.com/nomic-ai/gpt4all/pull/3228))

## [3.5.0-rc1] - 2024-12-04

### Added
- Add ability to attach text, markdown, and rst files to chat ([#3135](https://github.com/nomic-ai/gpt4all/pull/3135))
- Add feature to minimize to system tray (by [@bgallois](https://github.com/bgallois) in [#3109](https://github.com/nomic-ai/gpt4all/pull/3109))
- Basic cache for faster prefill when the input shares a prefix with previous context ([#3073](https://github.com/nomic-ai/gpt4all/pull/3073))
- Add ability to edit prompts and regenerate any response ([#3147](https://github.com/nomic-ai/gpt4all/pull/3147))

### Changed
- Implement Qt 6.8 compatibility ([#3121](https://github.com/nomic-ai/gpt4all/pull/3121))
- Use Jinja for chat templates instead of per-message QString.arg-style templates ([#3147](https://github.com/nomic-ai/gpt4all/pull/3147))
- API server: Use system message(s) from client instead of settings ([#3147](https://github.com/nomic-ai/gpt4all/pull/3147))
- API server: Accept messages in any order supported by the model instead of requiring user/assistant pairs ([#3147](https://github.com/nomic-ai/gpt4all/pull/3147))
- Remote models: Pass system message with "system" role instead of joining with user message ([#3147](https://github.com/nomic-ai/gpt4all/pull/3147))

### Removed
- Remove option to save binary model state to disk ([#3147](https://github.com/nomic-ai/gpt4all/pull/3147))

### Fixed
- Fix bug in GUI when localdocs encounters binary data ([#3137](https://github.com/nomic-ai/gpt4all/pull/3137))
- Fix LocalDocs bugs that prevented some docx files from fully chunking ([#3140](https://github.com/nomic-ai/gpt4all/pull/3140))
- Fix missing softmax that was causing crashes and effectively infinite temperature since 3.4.0 ([#3202](https://github.com/nomic-ai/gpt4all/pull/3202))

## [3.4.2] - 2024-10-16

### Fixed
- Limit bm25 retrieval to only specified collections ([#3083](https://github.com/nomic-ai/gpt4all/pull/3083))
- Fix bug removing documents because of a wrong case sensitive file suffix check ([#3083](https://github.com/nomic-ai/gpt4all/pull/3083))
- Fix bug with hybrid localdocs search where database would get out of sync ([#3083](https://github.com/nomic-ai/gpt4all/pull/3083))
- Fix GUI bug where the localdocs embedding device appears blank ([#3083](https://github.com/nomic-ai/gpt4all/pull/3083))
- Prevent LocalDocs from not making progress in certain cases ([#3094](https://github.com/nomic-ai/gpt4all/pull/3094))

## [3.4.1] - 2024-10-11

### Fixed
- Improve the Italian translation ([#3048](https://github.com/nomic-ai/gpt4all/pull/3048))
- Fix models.json cache location ([#3052](https://github.com/nomic-ai/gpt4all/pull/3052))
- Fix LocalDocs regressions caused by docx change ([#3079](https://github.com/nomic-ai/gpt4all/pull/3079))
- Fix Go code being highlighted as Java ([#3080](https://github.com/nomic-ai/gpt4all/pull/3080))

## [3.4.0] - 2024-10-08

### Added
- Add bm25 hybrid search to localdocs ([#2969](https://github.com/nomic-ai/gpt4all/pull/2969))
- LocalDocs support for .docx files ([#2986](https://github.com/nomic-ai/gpt4all/pull/2986))
- Add support for attaching Excel spreadsheet to chat ([#3007](https://github.com/nomic-ai/gpt4all/pull/3007), [#3028](https://github.com/nomic-ai/gpt4all/pull/3028))

### Changed
- Rebase llama.cpp on latest upstream as of September 26th ([#2998](https://github.com/nomic-ai/gpt4all/pull/2998))
- Change the error message when a message is too long ([#3004](https://github.com/nomic-ai/gpt4all/pull/3004))
- Simplify chatmodel to get rid of unnecessary field and bump chat version ([#3016](https://github.com/nomic-ai/gpt4all/pull/3016))
- Allow ChatLLM to have direct access to ChatModel for restoring state from text ([#3018](https://github.com/nomic-ai/gpt4all/pull/3018))
- Improvements to XLSX conversion and UI fix ([#3022](https://github.com/nomic-ai/gpt4all/pull/3022))

### Fixed
- Fix a crash when attempting to continue a chat loaded from disk ([#2995](https://github.com/nomic-ai/gpt4all/pull/2995))
- Fix the local server rejecting min\_p/top\_p less than 1 ([#2996](https://github.com/nomic-ai/gpt4all/pull/2996))
- Fix "regenerate" always forgetting the most recent message ([#3011](https://github.com/nomic-ai/gpt4all/pull/3011))
- Fix loaded chats forgetting context when there is a system prompt ([#3015](https://github.com/nomic-ai/gpt4all/pull/3015))
- Make it possible to downgrade and keep some chats, and avoid crash for some model types ([#3030](https://github.com/nomic-ai/gpt4all/pull/3030))
- Fix scroll positition being reset in model view, and attempt a better fix for the clone issue ([#3042](https://github.com/nomic-ai/gpt4all/pull/3042))

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

[3.6.1]: https://github.com/nomic-ai/gpt4all/compare/v3.6.0...v3.6.1
[3.6.0]: https://github.com/nomic-ai/gpt4all/compare/v3.5.3...v3.6.0
[3.5.3]: https://github.com/nomic-ai/gpt4all/compare/v3.5.2...v3.5.3
[3.5.2]: https://github.com/nomic-ai/gpt4all/compare/v3.5.1...v3.5.2
[3.5.1]: https://github.com/nomic-ai/gpt4all/compare/v3.5.0...v3.5.1
[3.5.0]: https://github.com/nomic-ai/gpt4all/compare/v3.5.0-rc2...v3.5.0
[3.5.0-rc2]: https://github.com/nomic-ai/gpt4all/compare/v3.5.0-rc1...v3.5.0-rc2
[3.5.0-rc1]: https://github.com/nomic-ai/gpt4all/compare/v3.4.2...v3.5.0-rc1
[3.4.2]: https://github.com/nomic-ai/gpt4all/compare/v3.4.1...v3.4.2
[3.4.1]: https://github.com/nomic-ai/gpt4all/compare/v3.4.0...v3.4.1
[3.4.0]: https://github.com/nomic-ai/gpt4all/compare/v3.3.0...v3.4.0
[3.3.1]: https://github.com/nomic-ai/gpt4all/compare/v3.3.0...v3.3.1
[3.3.0]: https://github.com/nomic-ai/gpt4all/compare/v3.2.1...v3.3.0
[3.2.1]: https://github.com/nomic-ai/gpt4all/compare/v3.2.0...v3.2.1
[3.2.0]: https://github.com/nomic-ai/gpt4all/compare/v3.1.1...v3.2.0
[3.1.1]: https://github.com/nomic-ai/gpt4all/compare/v3.1.0...v3.1.1
[3.1.0]: https://github.com/nomic-ai/gpt4all/compare/v3.0.0...v3.1.0
