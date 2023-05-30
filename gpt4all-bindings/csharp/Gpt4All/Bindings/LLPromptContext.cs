namespace Gpt4All.Bindings;

/// <summary>
/// Wrapper around the llmodel_prompt_context structure for holding the prompt context.
/// </summary>
/// <remarks>
/// The implementation takes care of all the memory handling of the raw logits pointer and the
/// raw tokens pointer.Attempting to resize them or modify them in any way can lead to undefined behavior
/// </remarks>
public unsafe class LLModelPromptContext
{
    private llmodel_prompt_context _ctx;

    internal ref llmodel_prompt_context UnderlyingContext => ref _ctx;

    public LLModelPromptContext()
    {
        _ctx = new();
    }

    /// <summary>
    /// logits of current context
    /// </summary>
    public Span<float> Logits => new(_ctx.logits, (int)_ctx.logits_size);

    /// <summary>
    /// the size of the raw logits vector
    /// </summary>
    public nuint LogitsSize
    {
        get => _ctx.logits_size;
        set => _ctx.logits_size = value;
    }

    /// <summary>
    /// current tokens in the context window
    /// </summary>
    public Span<int> Tokens => new(_ctx.tokens, (int)_ctx.tokens_size);

    /// <summary>
    /// the size of the raw tokens vector
    /// </summary>
    public nuint TokensSize
    {
        get => _ctx.tokens_size;
        set => _ctx.tokens_size = value;
    }

    /// <summary>
    /// top k logits to sample from
    /// </summary>
    public int TopK
    {
        get => _ctx.top_k;
        set => _ctx.top_k = value;
    }

    /// <summary>
    /// nucleus sampling probability threshold
    /// </summary>
    public float TopP
    {
        get => _ctx.top_p;
        set => _ctx.top_p = value;
    }

    /// <summary>
    /// temperature to adjust model's output distribution
    /// </summary>
    public float Temperature
    {
        get => _ctx.temp;
        set => _ctx.temp = value;
    }

    /// <summary>
    /// number of tokens in past conversation
    /// </summary>
    public int PastNum
    {
        get => _ctx.n_past;
        set => _ctx.n_past = value;
    }

    /// <summary>
    /// number of predictions to generate in parallel
    /// </summary>
    public int Batches
    {
        get => _ctx.n_batch;
        set => _ctx.n_batch = value;
    }

    /// <summary>
    /// number of tokens to predict
    /// </summary>
    public int TokensToPredict
    {
        get => _ctx.n_predict;
        set => _ctx.n_predict = value;
    }

    /// <summary>
    /// penalty factor for repeated tokens
    /// </summary>
    public float RepeatPenalty
    {
        get => _ctx.repeat_penalty;
        set => _ctx.repeat_penalty = value;
    }

    /// <summary>
    /// last n tokens to penalize
    /// </summary>
    public int RepeatLastN
    {
        get => _ctx.repeat_last_n;
        set => _ctx.repeat_last_n = value;
    }

    /// <summary>
    /// number of tokens possible in context window
    /// </summary>
    public int ContextSize
    {
        get => _ctx.n_ctx;
        set => _ctx.n_ctx = value;
    }

    /// <summary>
    /// percent of context to erase if we exceed the context window
    /// </summary>
    public float ContextErase
    {
        get => _ctx.context_erase;
        set => _ctx.context_erase = value;
    }
}
