using Gpt4All.Bindings;

namespace Gpt4All;

public static class LLPromptContextExtensions
{
    internal static string Dump(this LLModelPromptContext context)
    {
        var ctx = context.UnderlyingContext;
        return @$"
        {{
            logits_size = {ctx.logits_size}
            tokens_size = {ctx.tokens_size}
            n_past = {ctx.n_past}
            n_ctx = {ctx.n_ctx}
            n_predict = {ctx.n_predict}
            top_k = {ctx.top_k}
            top_p = {ctx.top_p}
            temp = {ctx.temp}
            n_batch = {ctx.n_batch}
            repeat_penalty = {ctx.repeat_penalty}
            repeat_last_n = {ctx.repeat_last_n}
            context_erase = {ctx.context_erase}
        }}";
    }

    public static void ApplyOptions(this LLModelPromptContext context, PredictRequestOptions opts)
    {
        context.TokensToPredict = opts.TokensToPredict;
        context.PastNum = opts.PastConversationTokensNum;
        context.TopK = opts.TopK;
        context.TopP = opts.TopP;
        context.Temperature = opts.Temperature;
        context.Batches = opts.Batches;
        context.RepeatLastN = opts.RepeatLastN;
        context.RepeatPenalty = opts.RepeatPenalty;
        context.Batches = opts.Batches;
        context.ContextErase = opts.ContextErase;
        context.ContextSize = opts.ContextSize;
    }
}
