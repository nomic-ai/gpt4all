# GPT4All Dart Binding

Dart bindings for the GPT4All C/C++ libraries and models.

https://pub.dev/packages/gpt4all

**Target platforms:**
- Windows
- macOS
- Linux

## Getting started

1. Compile `llmodel` C/C++ libraries

```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all
cd gpt4all/gpt4all-backend/
mkdir build
cd build
cmake ..
cmake --build . --parallel
```
Confirm that `libllmodel.*` exists in `gpt4all-backend/build`.

Those build artifacts contain all libraries required later in step 3.

2. Download model

Visit the [GPT4All Website](https://gpt4all.io/index.html) and use the Model Explorer
to find and download your model of choice (e.g. ggml-gpt4all-j-v1.3-groovy.bin).

3. Run the Dart code

Use the downloaded model and compiled libraries in your Dart code.
Have a look at the example implementation in [main.dart](example/main.dart):

```
  LLModel model = LLModel();
  try {
    // Always load the model before performing any other work.
    await model.load(
      // Path to the downloaded model file (*.bin)
      modelPath: '/some/path/to/ggml-gpt4all-j-v1.3-groovy.bin',
      // Path to the library folder including compiled *.dll (Windows), *.dylib (Mac) or
      // *.so (Linux) files
      librarySearchPath: '/some/path/gpt4all-backend/build',
      // Optionally fine-tune the default configuration
      promptConfig: LLModelPromptConfig()..nPredict = 256,
    );

    // Generate a response to the given prompt
    await model.generate(
      prompt: "### Human:\nWhat is the meaning of life?\n### Assistant:",
    );
  } finally {
    // Always destroy the model after calling the load(..) method
    model.destroy();
  }
```
