using System.Runtime.InteropServices;

namespace Gpt4All.Bindings
{
    internal partial struct llmodel_error
    {
#pragma warning disable CS0649 // Field 'llmodel_error.message' is never assigned to, and will always have its default value
        [NativeTypeName("const char *")] public IntPtr message;
#pragma warning restore CS0649 // Field 'llmodel_error.message' is never assigned to, and will always have its default value

#pragma warning disable CS0649 // Field 'llmodel_error.code' is never assigned to, and will always have its default value 0
        public int code;
#pragma warning restore CS0649 // Field 'llmodel_error.code' is never assigned to, and will always have its default value 0
    }

    internal unsafe partial struct llmodel_prompt_context
    {
        public float* logits;

        [NativeTypeName("size_t")] public nuint logits_size;

        [NativeTypeName("int32_t *")] public int* tokens;

        [NativeTypeName("size_t")] public nuint tokens_size;

        [NativeTypeName("int32_t")] public int n_past;

        [NativeTypeName("int32_t")] public int n_ctx;

        [NativeTypeName("int32_t")] public int n_predict;

        [NativeTypeName("int32_t")] public int top_k;

        public float top_p;

        public float temp;

        [NativeTypeName("int32_t")] public int n_batch;

        public float repeat_penalty;

        [NativeTypeName("int32_t")] public int repeat_last_n;

        public float context_erase;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate bool llmodel_prompt_callback([NativeTypeName("int32_t")] int token_id);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate bool llmodel_response_callback([NativeTypeName("int32_t")] int token_id,
        [NativeTypeName("const char *")] IntPtr response);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    internal delegate bool llmodel_recalculate_callback(bool is_recalculating);

    internal static unsafe partial class NativeMethods
    {
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public delegate bool LlmodelResponseCallback(int token_id,
            [MarshalAs(UnmanagedType.LPUTF8Str)] string response);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public delegate bool LlmodelPromptCallback(int token_id);

        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.I1)]
        public delegate bool LlmodelRecalculateCallback(bool isRecalculating);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true, BestFitMapping = false,
            ThrowOnUnmappableChar = true)]
        [return: NativeTypeName("llmodel_model")]
        public static extern IntPtr llmodel_model_create(
            [NativeTypeName("const char *")][MarshalAs(UnmanagedType.LPUTF8Str)] string model_path);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void llmodel_model_destroy([NativeTypeName("llmodel_model")] IntPtr model);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true, BestFitMapping = false,
            ThrowOnUnmappableChar = true)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool llmodel_loadModel(
            [NativeTypeName("llmodel_model")] IntPtr model,
            [NativeTypeName("const char *")] [MarshalAs(UnmanagedType.LPUTF8Str)]
            string model_path);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool llmodel_isModelLoaded([NativeTypeName("llmodel_model")] IntPtr model);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint64_t")]
        public static extern ulong llmodel_get_state_size([NativeTypeName("llmodel_model")] IntPtr model);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint64_t")]
        public static extern ulong llmodel_save_state_data([NativeTypeName("llmodel_model")] IntPtr model,
            [NativeTypeName("uint8_t *")] byte* dest);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("uint64_t")]
        public static extern ulong llmodel_restore_state_data([NativeTypeName("llmodel_model")] IntPtr model,
            [NativeTypeName("const uint8_t *")] byte* src);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true, BestFitMapping = false,
            ThrowOnUnmappableChar = true)]
        public static extern void llmodel_prompt(
            [NativeTypeName("llmodel_model")] IntPtr model,
            [NativeTypeName("const char *")] [MarshalAs(UnmanagedType.LPWStr)]
            string prompt,
            LlmodelPromptCallback prompt_callback,
            LlmodelResponseCallback response_callback,
            LlmodelRecalculateCallback recalculate_callback,
            ref llmodel_prompt_context ctx);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void llmodel_setThreadCount([NativeTypeName("llmodel_model")] IntPtr model,
            [NativeTypeName("int32_t")] int n_threads);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("int32_t")]
        public static extern int llmodel_threadCount([NativeTypeName("llmodel_model")] IntPtr model);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void llmodel_set_implementation_search_path([NativeTypeName("const char *")] IntPtr path);

        [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        [return: NativeTypeName("const char *")]
        public static extern IntPtr llmodel_get_implementation_search_path();
    }
}
