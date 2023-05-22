namespace Gpt4All;

public record PredictRequestOptions
{
    public nuint LogitsSize { get; init; } = 0;

    public nuint TokensSize { get; init; } = 0;

    public int PastConversationTokensNum { get; init; } = 0;

    public int ContextSize { get; init; } = 1024;

    public int TokensToPredict { get; init; } = 128;

    public int TopK { get; init; } = 40;

    public float TopP { get; init; } = 0.9f;

    public float Temperature { get; init; } = 0.1f;

    public int Batches { get; init; } = 8;

    public float RepeatPenalty { get; init; } = 1.2f;

    public int RepeatLastN { get; init; } = 10;

    public float ContextErase { get; init; } = 0.5f;

    public static readonly PredictRequestOptions Defaults = new();
}
