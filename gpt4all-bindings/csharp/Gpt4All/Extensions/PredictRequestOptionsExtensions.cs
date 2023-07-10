using Gpt4All.Bindings;

namespace Gpt4All;

public static class PredictRequestOptionsExtensions
{
    public static LLModelPromptContext ToPromptContext(this PredictRequestOptions opts)
    {
        return new LLModelPromptContext
        {
            LogitsSize = opts.LogitsSize,
            TokensSize = opts.TokensSize,
            TopK = opts.TopK,
            TopP = opts.TopP,
            PastNum = opts.PastConversationTokensNum,
            RepeatPenalty = opts.RepeatPenalty,
            Temperature = opts.Temperature,
            RepeatLastN = opts.RepeatLastN,
            Batches = opts.Batches,
            ContextErase = opts.ContextErase,
            ContextSize = opts.ContextSize,
            TokensToPredict = opts.TokensToPredict
        };
    }
}
