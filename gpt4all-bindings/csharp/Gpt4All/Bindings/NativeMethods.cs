using System.Runtime.InteropServices;

namespace Gpt4All.Bindings;

public unsafe partial struct llmodel_prompt_context
{
    public float* logits;

    [NativeTypeName("size_t")]
    public nuint logits_size;

    [NativeTypeName("int32_t *")]
    public int* tokens;

    [NativeTypeName("size_t")]
    public nuint tokens_size;

    [NativeTypeName("int32_t")]
    public int n_past;

    [NativeTypeName("int32_t")]
    public int n_ctx;

    [NativeTypeName("int32_t")]
    public int n_predict;

    [NativeTypeName("int32_t")]
    public int top_k;

    public float top_p;

    public float temp;

    [NativeTypeName("int32_t")]
    public int n_batch;

    public float repeat_penalty;

    [NativeTypeName("int32_t")]
    public int repeat_last_n;

    public float context_erase;
}
#pragma warning disable CA2101
internal static unsafe partial class NativeMethods
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    public delegate bool LlmodelResponseCallback(int token_id, [MarshalAs(UnmanagedType.LPUTF8Str)] string response);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    public delegate bool LlmodelPromptCallback(int token_id);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I1)]
    public delegate bool LlmodelRecalculateCallback(bool isRecalculating);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true, BestFitMapping = false, ThrowOnUnmappableChar = true)]
    [return: NativeTypeName("llmodel_model")]
    public static extern IntPtr llmodel_model_create2(
        [NativeTypeName("const char *")][MarshalAs(UnmanagedType.LPUTF8Str)] string model_path,
        [NativeTypeName("const char *")][MarshalAs(UnmanagedType.LPUTF8Str)] string build_variant,
        out IntPtr error);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void llmodel_model_destroy([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true, BestFitMapping = false, ThrowOnUnmappableChar = true)]
    [return: MarshalAs(UnmanagedType.I1)]
    public static extern bool llmodel_loadModel(
        [NativeTypeName("llmodel_model")] IntPtr model,
        [NativeTypeName("const char *")][MarshalAs(UnmanagedType.LPUTF8Str)] string model_path,
        [NativeTypeName("int32_t")] int n_ctx,
        [NativeTypeName("int32_t")] int ngl);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]

    [return: MarshalAs(UnmanagedType.I1)]
    public static extern bool llmodel_isModelLoaded([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint64_t")]
    public static extern ulong llmodel_get_state_size([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint64_t")]
    public static extern ulong llmodel_save_state_data([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("uint8_t *")] byte* dest);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint64_t")]
    public static extern ulong llmodel_restore_state_data([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("const uint8_t *")] byte* src);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true, BestFitMapping = false, ThrowOnUnmappableChar = true)]
    public static extern void llmodel_prompt(
        [NativeTypeName("llmodel_model")] IntPtr model,
        [NativeTypeName("const char *")][MarshalAs(UnmanagedType.LPUTF8Str)] string prompt,
        LlmodelPromptCallback prompt_callback,
        LlmodelResponseCallback response_callback,
        LlmodelRecalculateCallback recalculate_callback,
        ref llmodel_prompt_context ctx);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void llmodel_setThreadCount([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("int32_t")] int n_threads);

    [DllImport("libllmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("int32_t")]
    public static extern int llmodel_threadCount([NativeTypeName("llmodel_model")] IntPtr model);
}
#pragma warning restore CA2101
