import 'dart:ffi' as ffi;

class LLModelError extends ffi.Struct {

  external ffi.Pointer<ffi.Char> message;

  @ffi.Int32()
  external int status;
}
