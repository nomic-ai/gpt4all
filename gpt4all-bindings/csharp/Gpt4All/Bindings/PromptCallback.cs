using System.Runtime.InteropServices;

namespace Gpt4All.Bindings;

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate bool PromptCallback([NativeTypeName("int32_t")] int token_id);
