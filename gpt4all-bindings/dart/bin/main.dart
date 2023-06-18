import 'package:gpt4all_dart_binding/llmodel_generation_config.dart';
import 'package:gpt4all_dart_binding/llmodel.dart';

void main() async {
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

    // Generate a response to the given prompt
    await model.generate(
      prompt: "### Human:\nWhat is the meaning of life?\n### Assistant:",
      generationConfig: LLModelGenerationConfig()..nPredict = 256,
    );
  } finally {
    // Always destroy the model after calling the load(..) method
    model.destroy();
  }
}
