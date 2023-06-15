import 'package:gpt4all_dart_binding/llmodel.dart';

void main(List<String> arguments) {
  LLModel? model;
  try {
    model = LLModel(
      modelPath: '', // TODO path
    );
  } finally {
    model?.destroy(); // TODO use finalizer package
  }
}
