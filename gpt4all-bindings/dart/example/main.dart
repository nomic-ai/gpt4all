import 'dart:io';

import 'package:gpt4all/gpt4all.dart';

void main() async {
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
    );
  } finally {
    // Always destroy the model after calling the load(..) method
    model.destroy();
  }
}
