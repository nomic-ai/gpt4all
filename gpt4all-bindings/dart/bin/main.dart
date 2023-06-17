import 'package:gpt4all_dart_binding/generation_config.dart';
import 'package:gpt4all_dart_binding/llmodel.dart';

void main(List<String> arguments) async {
  LLModel? model;
  try {
    model = LLModel();
    await model.load(
      modelPath:
          '/Users/moritz.fink/Private/Coding/gpt4all/gpt4all-bindings/dart/model/ggml-gpt4all-j-v1.3-groovy.bin', // TODO path
    );
    model.generate(
      prompt: "### Human:\nWhat is the meaning of life\n### Assistant:",
      generationConfig: GenerationConfig(),
    );
  } finally {
    if (model?.isLoaded ?? false) {
      model?.destroy(); // TODO use finalizer package
    }
  }
}
