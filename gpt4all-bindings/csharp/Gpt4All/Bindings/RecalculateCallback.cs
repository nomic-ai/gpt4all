using System.Runtime.InteropServices;

namespace Gpt4All.Bindings;

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate bool RecalculateCallback(bool is_recalculating);
