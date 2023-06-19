import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:gpt4all/src/llmodel_generation_config.dart';
import 'package:gpt4all/src/llmodel_error.dart';
import 'package:gpt4all/src/llmodel_library.dart';
import 'package:gpt4all/src/llmodel_prompt_context.dart';

class LLModel {
  bool _isLoaded = false;

  late final LLModelLibrary _library;
  late final ffi.Pointer _model;

  /// Define a [callback] function for prompts, which returns a bool
  /// indicating whether the model should keep processing based on a
  /// given [tokenId].
  static void setPromptCallback(bool Function(int tokenId) callback) =>
      LLModelLibrary.promptCallback = callback;

  /// Define a [callback] function for responses, which returns a bool
  /// indicating whether the model should keep processing based on a
  /// given [tokenId] and [response] string.
  ///
  /// A [tokenId] of -1 indicates the string is an error string.
  static void setResponseCallback(
          bool Function(int tokenId, String response) callback) =>
      LLModelLibrary.responseCallback = callback;

  /// Define a [callback] function for recalculation, which returns a bool
  /// indicating whether the model should keep processing based on whether
  /// the model [isRecalculating] the context.
  static void setRecalculateCallback(
          bool Function(bool isRecalculating) callback) =>
      LLModelLibrary.recalculateCallback = callback;

  /// Load the model (.bin) from the [modelPath] and loads required libraries
  /// (.dll/.dylib/.so) from the [librarySearchPath] folder.
  ///
  /// This method must be called before any other interaction with the [LLModel].
  ///
  /// Make sure to call the [destroy] method once the work is performed.
  Future<void> load({
    required final String modelPath,
    required final String librarySearchPath,
  }) async {
    final ffi.Pointer<LLModelError> error = calloc<LLModelError>();
    try {
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
        error: error,
      );

      if (_model.address == ffi.nullptr.address) {
        final String errorMsg = error.ref.message.toDartString();
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
      calloc.free(error);
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

  /// Generate a response to the [prompt] using the model. The
  /// [LLModelGenerationConfig] can be used to optimize the model invocation.
  Future<void> generate({
    required String prompt,
    required LLModelGenerationConfig generationConfig,
  }) async {
    final logitsPtr = calloc<ffi.Pointer<ffi.Float>>();
    final tokensPtr = calloc<ffi.Pointer<ffi.Int32>>();
    final promptContextPtr = calloc<llmodel_prompt_context>();
    promptContextPtr.ref
      ..logits = logitsPtr.value // TODO generationConfig.logits
      ..logits_size = 0 // TODO generationConfig.logits.length
      ..tokens = tokensPtr.value // TODO generationConfig.tokens
      ..tokens_size = 0 // TODO generationConfig.tokens.length
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
      calloc.free(logitsPtr);
      calloc.free(tokensPtr);
    }
  }

  /// Destroy the model instance.
  ///
  /// Make sure to invoke this method once the model is no longer needed.
  void destroy() {
    if (_isLoaded) {
      _library.modelDestroy(
        model: _model,
      );
      _isLoaded = false;
    }
  }
}
