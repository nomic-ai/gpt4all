import 'dart:ffi' as ffi;
import 'package:ffi/ffi.dart';
import 'package:gpt4all_dart_binding/llmodel_error.dart';

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

typedef llmodel_set_implementation_search_path_func = ffi.Void Function(
    ffi.Pointer<Utf8>);
typedef LLModelSetImplementationSearchPath = void Function(
    ffi.Pointer<Utf8>);

class LLModelLibrary {
  late final ffi.DynamicLibrary _dynamicLibrary;

  // Dart methods binding to native methods
  late final LLModelIsModelLoaded _llModelIsModelLoaded;
  late final LLModelLoadModel _llModelLoadModel;
  late final LLModelModelCreate2 _llModelModelCreate2;
  late final LLModelModelDestroy _llModelModelDestroy;
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
    final ffi.Pointer result = _llModelModelCreate2(modelPath.toNativeUtf8(), buildVariant.toNativeUtf8(), error);
    malloc.free(modelPathNative);
    malloc.free(buildVariantNative);
    return result;
  }

  void modelDestroy({
    required ffi.Pointer model,
  }) {
    _llModelModelDestroy(model);
  }

  void setImplementationSearchPath({
    required String path,
  }) {
    final ffi.Pointer<Utf8> pathNative = path.toNativeUtf8();
    _llModelSetImplementationSearchPath(path.toNativeUtf8());
    malloc.free(pathNative);
  }
}
