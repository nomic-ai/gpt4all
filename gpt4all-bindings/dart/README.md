# GPT4All Dart Binding

## Getting started

1. Compile `llmodel` C/C++ sources

```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all
cd gpt4all/gpt4all-backend/
mkdir build
cd build
cmake ..
cmake --build . --parallel
```
Confirm that `libllmodel.*` exists in `gpt4all-backend/build`.

2. Run the model

Use the compiled model and libraries in your Dart code.
Have a look at the example implementation in [main.dart](bin/main.dart):

```
  LLModel model = LLModel();
  try {
    // Always load the model before performing any other work.
    await model.load(
      // Path to the model file (*.bin)
      modelPath:
          '/Users/moritz.fink/Private/Coding/gpt4all/gpt4all-bindings/dart/model/ggml-gpt4all-j-v1.3-groovy.bin',
      // Path to the library folder including *.dll (Windows), *.dylib (Mac) or
      // *.so (Linux) files
      librarySearchPath:
          '/Users/moritz.fink/Private/Coding/gpt4all/gpt4all-bindings/dart/shared_libs',
    );

    // Optional: Define what to do with prompt responses
    // Prints to stdout if not defined
    // For demo purposes we print to stderr here
    LLModel.setResponseCallback(
      (int tokenId, String response) {
        stderr.write(response);
        return true;
      },
    );

    // Generate a response to the given prompt
    await model.generate(
      prompt: "### Human:\nWhat is the meaning of life?\n### Assistant:",
      generationConfig: LLModelGenerationConfig()..nPredict = 256,
    );
  } finally {
    // Always destroy the model after calling the load(..) method
    model.destroy();
  }
```
