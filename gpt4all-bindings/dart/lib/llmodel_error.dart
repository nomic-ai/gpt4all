import 'dart:ffi';

import 'package:ffi/ffi.dart';

class LLModelError extends Struct {

  external Pointer<Utf8> message; // TODO does this work?

  @Int32()
  external int status;

  // TODO constructor with runtime?
}
