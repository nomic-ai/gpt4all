namespace Gpt4All.Bindings;

internal unsafe partial struct PromptContext
{
    public float* logits;

    [NativeTypeName("size_t")]
    public UIntPtr logits_size;

    [NativeTypeName("int32_t *")]
    public int* tokens;

    [NativeTypeName("size_t")]
    public UIntPtr tokens_size;

    [NativeTypeName("int32_t")]
    public int n_past;

    [NativeTypeName("int32_t")]
    public int n_ctx;

    [NativeTypeName("int32_t")]
    public int n_predict;

    [NativeTypeName("int32_t")]
    public int top_k;

    public float top_p;

    public float min_p;

    public float temp;

    [NativeTypeName("int32_t")]
    public int n_batch;

    public float repeat_penalty;

    [NativeTypeName("int32_t")]
    public int repeat_last_n;

    public float context_erase;
}
