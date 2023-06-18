import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:gpt4all_dart_binding/llmodel_generation_config.dart';
import 'package:gpt4all_dart_binding/llmodel_error.dart';
import 'package:gpt4all_dart_binding/llmodel_library.dart';
import 'package:gpt4all_dart_binding/llmodel_prompt_context.dart';

class LLModel {
  bool _isLoaded = false;

  late final ffi.Pointer<LLModelError> _error;

  late final LLModelLibrary _library;
  late final ffi.Pointer _model;

  Future<void> load({
    required final String modelPath,
    required final String librarySearchPath,
  }) async {
    try {
      _error = calloc<LLModelError>();

      _library = LLModelLibrary(
        pathToLibrary: '$librarySearchPath/libllmodel${_getFileSuffix()}',
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

  String _getFileSuffix() {
    if (Platform.isWindows) {
      return '.dll';
    } else if (Platform.isMacOS) {
      return '.dylib';
    } else if (Platform.isLinux) {
      return '.so';
    } else {
      throw Exception('Unsupported device');
    }
  }

  Future<void> generate({
    required String prompt,
    required LLModelGenerationConfig generationConfig,
  }) async {
    final promptContextPtr = calloc<llmodel_prompt_context>();
    promptContextPtr.ref
      //TODO logits
      ..logits_size = generationConfig.logits.length
      //TODO tokens
      ..tokens_size = generationConfig.tokens.length
      ..n_past = generationConfig.nPast
      ..n_ctx = generationConfig.nCtx
      ..n_predict = generationConfig.nPredict
      ..top_k = generationConfig.topK
      ..top_p = generationConfig.topP
      ..temp = generationConfig.temp
      ..n_batch = generationConfig.nBatch
      ..repeat_penalty = generationConfig.repeatPenalty
      ..repeat_last_n = generationConfig.repeatLastN
      ..context_erase = generationConfig.contextErase;

    try {
      _library.prompt(
        model: _model,
        prompt: prompt,
        promptContext: promptContextPtr,
      );
    } finally {
      calloc.free(promptContextPtr);
    }
  }

  void destroy() {
    if (_isLoaded) {
      _library.modelDestroy(
        model: _model,
      );
      _isLoaded = false;
    }
  }
}
