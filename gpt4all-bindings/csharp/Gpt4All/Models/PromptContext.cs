namespace Gpt4All.Models;

/// <summary>
/// Structure for holding the prompt context.
/// </summary>
public class PromptContext
{
    public PromptContext()
    {
        NCtx = 2048;
        NPredict = 100;
        NBatch = 1;
        TopK = 0;
        TopP = 0.0f;
        MinP = 0.0f;
        Temp = 1.0f;
        RepeatPenalty = 1.0f;
        RepeatLastN = 0;
        ContextErase = 0.0f;
    }

    /// <summary>
    /// logits of current context
    /// </summary>
    public float[]? Logits { get; set; }

    /// <summary>
    /// current tokens in the context window
    /// </summary>
    public int[]? Tokens { get; set; }

    /// <summary>
    /// number of tokens in past conversation
    /// </summary>
    public int NPast { get; set; }

    /// <summary>
    /// number of tokens possible in context window
    /// </summary>
    public int NCtx { get; set; }

    /// <summary>
    /// number of tokens to predict
    /// </summary>
    public int NPredict { get; set; }

    /// <summary>
    /// top k logits to sample from
    /// </summary>
    public int TopK { get; set; }

    /// <summary>
    /// nucleus sampling probability threshold
    /// </summary>
    public float TopP { get; set; }

    /// <summary>
    /// Min P sampling
    /// </summary>
    public float MinP { get; set; }

    /// <summary>
    /// temperature to adjust model's output distribution
    /// </summary>
    public float Temp { get; set; }

    /// <summary>
    /// number of predictions to generate in parallel
    /// </summary>
    public int NBatch { get; set; }

    /// <summary>
    /// penalty factor for repeated tokens
    /// </summary>
    public float RepeatPenalty { get; set; }

    /// <summary>
    /// last n tokens to penalize
    /// </summary>
    public int RepeatLastN { get; set; }

    /// <summary>
    /// percent of context to erase if we exceed the context window
    /// </summary>
    public float ContextErase { get; set; }
}
