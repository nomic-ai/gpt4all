# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- New Feature: Generate suggested follow-up questions. ([#2634](https://github.com/nomic-ai/gpt4all/pull/2634))
- Add scaffolding for translations. ([#2612](https://github.com/nomic-ai/gpt4all/pull/2612))
- Add Spanish (MX) Translation (by @jstayco in [#2654](https://github.com/nomic-ai/gpt4all/pull/2654))
- Add zh_CN.ts translation. ([#2657](https://github.com/nomic-ai/gpt4all/pull/2657))
- Feature: dynamic changes of language and locale at runtime issue #2644 ([#2659](https://github.com/nomic-ai/gpt4all/pull/2659), [#2677](https://github.com/nomic-ai/gpt4all/pull/2677))
- Add a romanian translation file submitted by @SINAPSA_IC ([#2662](https://github.com/nomic-ai/gpt4all/pull/2662))
- Add zh_TW translation (by @supersonictw in [#2661](https://github.com/nomic-ai/gpt4all/pull/2661))

### Changed
- Customize combo boxes and context menus to fit the new style ([#2535](https://github.com/nomic-ai/gpt4all/pull/2535))
- Improve view bar scaling and Model Settings layout ([#2520](https://github.com/nomic-ai/gpt4all/pull/2520)
- A better animation for when the model is thinking/responding. ([#2557](https://github.com/nomic-ai/gpt4all/pull/2557))
- GPT4All Chat server API: add errors 405 Method Not Allowed (by @cosmic-snow in [#2615](https://github.com/nomic-ai/gpt4all/pull/2615))
- Update theme for menus (by @3Simplex in [#2578](https://github.com/nomic-ai/gpt4all/pull/2578))
- Make the stop generation button visible during model response. ([#2561](https://github.com/nomic-ai/gpt4all/pull/2561))
- Try using CUDA 11.8 for all Circle CI builds. ([#2639](https://github.com/nomic-ai/gpt4all/pull/2639))
- Add ability to click on links in latest news. ([#2643](https://github.com/nomic-ai/gpt4all/pull/2643))
- Fixup ChatTheme and FontSize to use enums which enables translation. ([#2667](https://github.com/nomic-ai/gpt4all/pull/2667))
- Improve LocalDocs view's error message (by @cosmic-snow in [#2679](https://github.com/nomic-ai/gpt4all/pull/2679))
- Fix settings translations ([#2690](https://github.com/nomic-ai/gpt4all/pull/2690))
- backend: rebase llama.cpp submodule on latest upstream ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694))

### Deprecated

### Removed
- Remove support for GPT-J models. ([#2676](https://github.com/nomic-ai/gpt4all/pull/2676), [#2693](https://github.com/nomic-ai/gpt4all/pull/2693))

### Fixed
- Fix placement of our thumbs down dialog and datalake opt-in dialog. ([#2540](https://github.com/nomic-ai/gpt4all/pull/2540))
- Fix folder dialog on linux so that we can select the folder properly. ([#2541](https://github.com/nomic-ai/gpt4all/pull/2541))
- modellist: work around filtered item models getting out of sync ([#2545](https://github.com/nomic-ai/gpt4all/pull/2545))
- Fix scrolling of the chat view at expense of some more memory usage. ([#2555](https://github.com/nomic-ai/gpt4all/pull/2555))
- Fix "reload" showing for models that no longer exist ([#2520](https://github.com/nomic-ai/gpt4all/pull/2520)
- Fix property binding loop warning. ([#2601](https://github.com/nomic-ai/gpt4all/pull/2601))
- Fixes for issue #2519. ([#2543](https://github.com/nomic-ai/gpt4all/pull/2543))
- llama.cpp: update submodule for CPU fallback fix ([#2640](https://github.com/nomic-ai/gpt4all/pull/2640))
- Case-insentitive file extensions ([#2642](https://github.com/nomic-ai/gpt4all/pull/2642), [#2684](https://github.com/nomic-ai/gpt4all/pull/2684))

[Unreleased]: https://github.com/nomic-ai/gpt4all/compare/v3.0.0...HEAD
