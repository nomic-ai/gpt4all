import 'package:gpt4all_dart_binding/llmodel.dart';

void main(List<String> arguments) async {
  LLModel? model;
  try {
    model = LLModel();
    await model.load(
      modelPath: '', // TODO path
    );
  } finally {
    model?.destroy(); // TODO use finalizer package
  }
}
