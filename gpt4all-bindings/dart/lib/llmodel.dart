import 'dart:ffi' as ffi;
import 'dart:js_interop';
import 'package:ffi/ffi.dart';
import 'package:gpt4all_dart_binding/llmodel_error.dart';
import 'package:gpt4all_dart_binding/llmodel_library.dart';
import 'package:gpt4all_dart_binding/llmodel_util.dart';

class LLModel {
  final ffi.Pointer<LLModelError> _error = calloc<LLModelError>();

  late final LLModelLibrary _library;
  late final ffi.Pointer _model;

  LLModel({
    required String modelPath,
  }) {
    try {
      String librarySearchPath = LLModelUtil.copySharedLibraries();

      _library = LLModelLibrary(
        pathToLibrary: librarySearchPath,
      );

      _library.setImplementationSearchPath(
        path: librarySearchPath,
      );

      // TODO check if model file exists

      _model = _library.modelCreate2(
        modelPath: modelPath,
        buildVariant: "auto",
        error: _error.ref,
      );

      if (_model.isUndefinedOrNull) {
        throw Exception(
            "Could not load gpt4all backend: ${_error.ref.message}");
      }

      _library.loadModel(
        model: _model,
        modelPath: modelPath,
      );

      if (!_library.isModelLoaded(model: _model)) {
        throw Exception("The model could not be loaded");
      }
    } finally {
      calloc.free(_error);
    }
  }

  void destroy() {
    _library.modelDestroy(
      model: _model,
    );
  }
}
