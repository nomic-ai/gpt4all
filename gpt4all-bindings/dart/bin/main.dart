import 'package:gpt4all_dart_binding/llmodel_generation_config.dart';
import 'package:gpt4all_dart_binding/llmodel.dart';

void main() async {
  LLModel model = LLModel();
  try {
    await model.load(
      // TODO paths
      modelPath:
          '/Users/moritz.fink/Private/Coding/gpt4all/gpt4all-bindings/dart/model/ggml-gpt4all-j-v1.3-groovy.bin',
      librarySearchPath:
          '/Users/moritz.fink/Private/Coding/gpt4all/gpt4all-bindings/dart/shared_libs',
    );
    await model.generate(
      prompt: "### Human:\nWhat is the meaning of life?\n### Assistant:",
      generationConfig: LLModelGenerationConfig(),
    );
  } finally {
    model.destroy();
  }
}
