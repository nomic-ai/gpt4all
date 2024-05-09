using System.Runtime.InteropServices;

namespace Gpt4All.Bindings;

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal unsafe delegate bool llmodel_emb_cancel_callback([NativeTypeName("unsigned int *")] uint* batch_sizes, [NativeTypeName("unsigned int")] uint n_batch, [NativeTypeName("const char *")] IntPtr backend);
