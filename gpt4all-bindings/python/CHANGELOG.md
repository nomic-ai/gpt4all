# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Add support for GPT-NeoX, Gemma 2, OpenELM, ChatGLM, and Jais architectures (all with Vulkan support) ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694))
- Enable Vulkan support for StarCoder2, XVERSE, Command R, and OLMo ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694))
- Add support for DeepSeek-V2 architecture (no Vulkan support) ([#2702](https://github.com/nomic-ai/gpt4all/pull/2702))
- Add Llama 3.1 8B Instruct to models3.json (by [@3Simplex](https://github.com/3Simplex) in [#2731](https://github.com/nomic-ai/gpt4all/pull/2731) and [#2732](https://github.com/nomic-ai/gpt4all/pull/2732))
- Support Llama 3.1 RoPE scaling ([#2758](https://github.com/nomic-ai/gpt4all/pull/2758))
- Add Qwen2-1.5B-Instruct to models3.json (by [@ThiloteE](https://github.com/ThiloteE) in [#2759](https://github.com/nomic-ai/gpt4all/pull/2759))

### Changed
- Update llama.cpp to commit 87e397d00 from July 19th ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694))

### Removed
- Remove unused internal llmodel\_has\_gpu\_device ([#2409](https://github.com/nomic-ai/gpt4all/pull/2409))
- Remove support for GPT-J models ([#2676](https://github.com/nomic-ai/gpt4all/pull/2676), [#2693](https://github.com/nomic-ai/gpt4all/pull/2693))

### Fixed
- Fix debug mode crash on Windows and undefined behavior in LLamaModel::embedInternal ([#2467](https://github.com/nomic-ai/gpt4all/pull/2467))
- Fix CUDA PTX errors with some GPT4All builds ([#2421](https://github.com/nomic-ai/gpt4all/pull/2421))
- Fix mishandling of inputs greater than n\_ctx tokens after [#1970](https://github.com/nomic-ai/gpt4all/pull/1970) ([#2498](https://github.com/nomic-ai/gpt4all/pull/2498))
- Fix crash when Kompute falls back to CPU ([#2640](https://github.com/nomic-ai/gpt4all/pull/2640))
- Fix several Kompute resource management issues ([#2694](https://github.com/nomic-ai/gpt4all/pull/2694))
- Fix crash/hang when some models stop generating, by showing special tokens ([#2701](https://github.com/nomic-ai/gpt4all/pull/2701))
- Fix several backend issues ([#2778](https://github.com/nomic-ai/gpt4all/pull/2778))
  - Restore leading space removal logic that was incorrectly removed in [#2694](https://github.com/nomic-ai/gpt4all/pull/2694)
  - CUDA: Cherry-pick llama.cpp DMMV cols requirement fix that caused a crash with long conversations since [#2694](https://github.com/nomic-ai/gpt4all/pull/2694)

[Unreleased]: https://github.com/nomic-ai/gpt4all/compare/python-v2.7.0...HEAD
