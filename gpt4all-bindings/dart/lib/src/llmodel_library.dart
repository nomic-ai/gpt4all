import 'dart:ffi' as ffi;
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:gpt4all/src/llmodel_error.dart';
import 'package:gpt4all/src/llmodel_prompt_context.dart';

typedef llmodel_isModelLoaded_func = ffi.Bool Function(ffi.Pointer);
typedef LLModelIsModelLoaded = bool Function(ffi.Pointer);

typedef llmodel_loadModel_func = ffi.Bool Function(
    ffi.Pointer, ffi.Pointer<Utf8>);
typedef LLModelLoadModel = bool Function(ffi.Pointer, ffi.Pointer<Utf8>);

typedef llmodel_model_create2_func = ffi.Pointer Function(
    ffi.Pointer<Utf8>, ffi.Pointer<Utf8>, ffi.Pointer<LLModelError>);
typedef LLModelModelCreate2 = ffi.Pointer Function(
    ffi.Pointer<Utf8>, ffi.Pointer<Utf8>, ffi.Pointer<LLModelError>);

typedef llmodel_model_destroy_func = ffi.Void Function(ffi.Pointer);
typedef LLModelModelDestroy = void Function(ffi.Pointer);

typedef llmodel_prompt_callback_func = ffi.Bool Function(ffi.Int32);
typedef LLModelPromptCallback = bool Function(int);

typedef llmodel_response_callback_func = ffi.Bool Function(
    ffi.Int32, ffi.Pointer<Utf8>);
typedef LLModelResponseCallback = bool Function(int, ffi.Pointer<Utf8>);

typedef llmodel_recalculate_callback_func = ffi.Bool Function(ffi.Bool);
typedef LLModelRecalculateCallback = bool Function(bool);

typedef llmodel_prompt_func = ffi.Void Function(
    ffi.Pointer,
    ffi.Pointer<Utf8>,
    ffi.Pointer<ffi.NativeFunction<llmodel_prompt_callback_func>>,
    ffi.Pointer<ffi.NativeFunction<llmodel_response_callback_func>>,
    ffi.Pointer<ffi.NativeFunction<llmodel_recalculate_callback_func>>,
    ffi.Pointer<llmodel_prompt_context>);
typedef LLModelPrompt = void Function(
    ffi.Pointer,
    ffi.Pointer<Utf8>,
    ffi.Pointer<ffi.NativeFunction<llmodel_prompt_callback_func>>,
    ffi.Pointer<ffi.NativeFunction<llmodel_response_callback_func>>,
    ffi.Pointer<ffi.NativeFunction<llmodel_recalculate_callback_func>>,
    ffi.Pointer<llmodel_prompt_context>);

typedef llmodel_set_implementation_search_path_func = ffi.Void Function(
    ffi.Pointer<Utf8>);
typedef LLModelSetImplementationSearchPath = void Function(ffi.Pointer<Utf8>);

class LLModelLibrary {
  static const bool except = false;

  static bool Function(int) promptCallback = (int tokenId) => true;
  static bool Function(int, String) responseCallback =
      (int tokenId, String response) {
    if (tokenId == -1) {
      stderr.write(response);
    } else {
      stdout.write(response);
    }
    return true;
  };
  static bool Function(bool) recalculateCallback =
      (bool isRecalculating) => isRecalculating;

  late final ffi.DynamicLibrary _dynamicLibrary;

  // Dart methods binding to native methods
  late final LLModelIsModelLoaded _llModelIsModelLoaded;
  late final LLModelLoadModel _llModelLoadModel;
  late final LLModelModelCreate2 _llModelModelCreate2;
  late final LLModelModelDestroy _llModelModelDestroy;
  late final LLModelPrompt _llModelPrompt;
  late final LLModelSetImplementationSearchPath
      _llModelSetImplementationSearchPath;

  LLModelLibrary({
    required String pathToLibrary,
  }) {
    _dynamicLibrary = ffi.DynamicLibrary.open(pathToLibrary);
    _initializeMethodBindings();
  }

  void _initializeMethodBindings() {
    _llModelIsModelLoaded = _dynamicLibrary
        .lookup<ffi.NativeFunction<llmodel_isModelLoaded_func>>(
            'llmodel_isModelLoaded')
        .asFunction();

    _llModelLoadModel = _dynamicLibrary
        .lookup<ffi.NativeFunction<llmodel_loadModel_func>>('llmodel_loadModel')
        .asFunction();

    _llModelModelCreate2 = _dynamicLibrary
        .lookup<ffi.NativeFunction<llmodel_model_create2_func>>(
            'llmodel_model_create2')
        .asFunction();

    _llModelModelDestroy = _dynamicLibrary
        .lookup<ffi.NativeFunction<llmodel_model_destroy_func>>(
            'llmodel_model_destroy')
        .asFunction();

    _llModelPrompt = _dynamicLibrary
        .lookup<ffi.NativeFunction<llmodel_prompt_func>>('llmodel_prompt')
        .asFunction();

    _llModelSetImplementationSearchPath = _dynamicLibrary
        .lookup<
                ffi.NativeFunction<
                    llmodel_set_implementation_search_path_func>>(
            'llmodel_set_implementation_search_path')
        .asFunction();
  }

  bool isModelLoaded({
    required ffi.Pointer model,
  }) {
    return _llModelIsModelLoaded(model);
  }

  bool loadModel({
    required ffi.Pointer model,
    required String modelPath,
  }) {
    final ffi.Pointer<Utf8> modelPathNative = modelPath.toNativeUtf8();
    final bool result = _llModelLoadModel(model, modelPathNative);
    malloc.free(modelPathNative);
    return result;
  }

  ffi.Pointer modelCreate2({
    required String modelPath,
    required String buildVariant,
    required ffi.Pointer<LLModelError> error,
  }) {
    final ffi.Pointer<Utf8> modelPathNative = modelPath.toNativeUtf8();
    final ffi.Pointer<Utf8> buildVariantNative = buildVariant.toNativeUtf8();
    final ffi.Pointer result = _llModelModelCreate2(
        modelPath.toNativeUtf8(), buildVariant.toNativeUtf8(), error);
    malloc.free(modelPathNative);
    malloc.free(buildVariantNative);
    return result;
  }

  void modelDestroy({
    required ffi.Pointer model,
  }) {
    _llModelModelDestroy(model);
  }

  void prompt({
    required ffi.Pointer model,
    required String prompt,
    required ffi.Pointer<llmodel_prompt_context> promptContext,
  }) {
    final ffi.Pointer<Utf8> promptNative = prompt.toNativeUtf8();
    _llModelPrompt(
      model,
      promptNative,
      ffi.Pointer.fromFunction<llmodel_prompt_callback_func>(
          _promptCallback, except),
      ffi.Pointer.fromFunction<llmodel_response_callback_func>(
          _responseCallback, except),
      ffi.Pointer.fromFunction<llmodel_recalculate_callback_func>(
          _recalculateCallback, except),
      promptContext,
    );
    malloc.free(promptNative);
  }

  static bool _promptCallback(int tokenId) => promptCallback(tokenId);

  static bool _responseCallback(int tokenId, ffi.Pointer<Utf8> responsePart) {
    return responseCallback(tokenId, responsePart.toDartString());
  }

  static bool _recalculateCallback(bool isRecalculating) =>
      recalculateCallback(isRecalculating);

  void setImplementationSearchPath({
    required String path,
  }) {
    final ffi.Pointer<Utf8> pathNative = path.toNativeUtf8();
    _llModelSetImplementationSearchPath(path.toNativeUtf8());
    malloc.free(pathNative);
  }
}
