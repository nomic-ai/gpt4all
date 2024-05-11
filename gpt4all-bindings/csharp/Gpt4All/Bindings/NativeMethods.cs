using System.Runtime.InteropServices;

namespace Gpt4All.Bindings;

internal static unsafe partial class NativeMethods
{
    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("llmodel_model")]
    [Obsolete]
    public static extern IntPtr llmodel_model_create([NativeTypeName("const char *")] IntPtr model_path);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("llmodel_model")]
    public static extern IntPtr llmodel_model_create2([NativeTypeName("const char *")] IntPtr model_path, [NativeTypeName("const char *")] IntPtr build_variant, [NativeTypeName("const char **")] IntPtr* error);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void llmodel_model_destroy([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("size_t")]
    public static extern UIntPtr llmodel_required_mem([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("const char *")] IntPtr model_path, int n_ctx, int ngl);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("bool")]
    public static extern byte llmodel_loadModel([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("const char *")] IntPtr model_path, int n_ctx, int ngl);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("bool")]
    public static extern byte llmodel_isModelLoaded([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint64_t")]
    public static extern ulong llmodel_get_state_size([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint64_t")]
    public static extern ulong llmodel_save_state_data([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("uint8_t *")] byte* dest);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("uint64_t")]
    public static extern ulong llmodel_restore_state_data([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("const uint8_t *")] byte* src);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void llmodel_prompt([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("const char *")] IntPtr prompt, [NativeTypeName("const char *")] IntPtr prompt_template, [NativeTypeName("llmodel_prompt_callback")] PromptCallback prompt_callback, [NativeTypeName("llmodel_response_callback")] ResponseCallback response_callback, [NativeTypeName("llmodel_recalculate_callback")] RecalculateCallback recalculate_callback, [NativeTypeName("llmodel_prompt_context *")] PromptContext* ctx, [NativeTypeName("bool")] byte special, [NativeTypeName("const char *")] IntPtr fake_reply);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern float* llmodel_embed([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("const char **")] IntPtr* texts, [NativeTypeName("size_t *")] UIntPtr* embedding_size, [NativeTypeName("const char *")] IntPtr prefix, int dimensionality, [NativeTypeName("size_t *")] UIntPtr* token_count, [NativeTypeName("bool")] byte do_mean, [NativeTypeName("bool")] byte atlas, [NativeTypeName("llmodel_emb_cancel_callback")] EmbCancelCallback cancel_cb, [NativeTypeName("const char **")] IntPtr* error);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void llmodel_free_embedding(float* ptr);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void llmodel_setThreadCount([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("int32_t")] int n_threads);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("int32_t")]
    public static extern int llmodel_threadCount([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    public static extern void llmodel_set_implementation_search_path([NativeTypeName("const char *")] IntPtr path);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("const char *")]
    public static extern IntPtr llmodel_get_implementation_search_path();

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("struct llmodel_gpu_device *")]
    public static extern GPUDevice* llmodel_available_gpu_devices([NativeTypeName("size_t")] UIntPtr memoryRequired, int* num_devices);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("bool")]
    public static extern byte llmodel_gpu_init_gpu_device_by_string([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("size_t")] UIntPtr memoryRequired, [NativeTypeName("const char *")] IntPtr device);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("bool")]
    public static extern byte llmodel_gpu_init_gpu_device_by_struct([NativeTypeName("llmodel_model")] IntPtr model, [NativeTypeName("const llmodel_gpu_device *")] GPUDevice* device);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("bool")]
    public static extern byte llmodel_gpu_init_gpu_device_by_int([NativeTypeName("llmodel_model")] IntPtr model, int device);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("bool")]
    public static extern byte llmodel_has_gpu_device([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("const char *")]
    public static extern IntPtr llmodel_model_backend_name([NativeTypeName("llmodel_model")] IntPtr model);

    [DllImport("llmodel", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
    [return: NativeTypeName("const char *")]
    public static extern IntPtr llmodel_model_gpu_device_name([NativeTypeName("llmodel_model")] IntPtr model);
}
