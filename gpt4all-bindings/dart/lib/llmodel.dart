import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:gpt4all_dart_binding/llmodel_error.dart';
import 'package:gpt4all_dart_binding/llmodel_library.dart';
import 'package:gpt4all_dart_binding/llmodel_util.dart';

class LLModel {
  bool _isLoaded = false;

  bool get isLoaded => _isLoaded;

  late final ffi.Pointer<LLModelError> _error;

  late final LLModelLibrary _library;
  late final ffi.Pointer _model;

  Future<void> load({
    required final String modelPath,
  }) async {
    try {
      _error = calloc<LLModelError>();

      final String librarySearchPath = LLModelUtil.copySharedLibraries();

      _library = LLModelLibrary(
        pathToLibrary: '$librarySearchPath/libllmodel.0.2.0.dylib', // TODO
      );

      _library.setImplementationSearchPath(
        path: librarySearchPath,
      );

      if (!File(modelPath).existsSync()) {
        throw Exception("Model file does not exist: $modelPath");
      }

      _model = _library.modelCreate2(
        modelPath: modelPath,
        buildVariant: "auto",
        error: _error,
      );

      if (_model.address == ffi.nullptr.address) {
        final String errorMsg = _error.ref.message.toDartString();
        throw Exception("Could not load gpt4all backend: $errorMsg");
      }

      _library.loadModel(
        model: _model,
        modelPath: modelPath,
      );

      if (_library.isModelLoaded(model: _model)) {
        _isLoaded = true;
      } else {
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
    _isLoaded = false;
  }
}
