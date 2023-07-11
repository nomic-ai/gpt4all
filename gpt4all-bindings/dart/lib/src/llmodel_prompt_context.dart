import 'dart:ffi' as ffi;

/// llmodel\_prompt\_context structure for holding the prompt context.
final class llmodel_prompt_context extends ffi.Struct {

  /// logits of current context
  external ffi.Pointer<ffi.Float> logits;

  /// the size of the raw logits vector
  @ffi.Size()
  external int logits_size;

  /// current tokens in the context window
  external ffi.Pointer<ffi.Int32> tokens;

  /// the size of the raw tokens vector
  @ffi.Size()
  external int tokens_size;

  /// number of tokens in past conversation
  @ffi.Int32()
  external int n_past;

  /// number of tokens possible in context window
  @ffi.Int32()
  external int n_ctx;

  /// number of tokens to predict
  @ffi.Int32()
  external int n_predict;

  /// top k logits to sample from
  @ffi.Int32()
  external int top_k;

  /// nucleus sampling probability threshold
  @ffi.Float()
  external double top_p;

  /// temperature to adjust model's output distribution
  @ffi.Float()
  external double temp;

  /// number of predictions to generate in parallel
  @ffi.Int32()
  external int n_batch;

  /// penalty factor for repeated tokens
  @ffi.Float()
  external double repeat_penalty;

  /// last n tokens to penalize
  @ffi.Int32()
  external int repeat_last_n;

  /// percent of context to erase if we exceed the context window
  @ffi.Float()
  external double context_erase;
}
