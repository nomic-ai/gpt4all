using Gpt4All.Bindings;

namespace Gpt4All;

internal static class LLPromptContextExtensions
{
    public static string Dump(this LLModelPromptContext context)
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
}
