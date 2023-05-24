using Gpt4All.Bindings;
using Gpt4All.Extensions;

namespace Gpt4All;

public class Gpt4All : IGpt4AllModel
{
    private readonly ILLModel _model;

    /// <inheritdoc/>
    public IPromptFormatter? PromptFormatter { get; set; }

    internal Gpt4All(ILLModel model)
    {
        _model = model;
        PromptFormatter = new DefaultPromptFormatter();
    }

    private string FormatPrompt(string prompt)
    {
        if (PromptFormatter == null) return prompt;

        return PromptFormatter.FormatPrompt(prompt);
    }

    public Task<ITextPredictionResult> GetPredictionAsync(string text, PredictRequestOptions opts, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(text);

        return Task.Run(() =>
        {
            var result = new TextPredictionResult();
            var context = opts.ToPromptContext();
            var prompt = FormatPrompt(text);

            _model.Prompt(prompt, context, responseCallback: e =>
            {
                if (e.IsError)
                {
                    result.Success = false;
                    result.ErrorMessage = e.Response;
                    return false;
                }
                result.Append(e.Response);
                return true;
            }, cancellationToken: cancellationToken);

            return (ITextPredictionResult)result;
        }, CancellationToken.None);
    }

    public Task<ITextPredictionStreamingResult> GetStreamingPredictionAsync(string text, PredictRequestOptions opts, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(text);

        var result = new TextPredictionStreamingResult();

        _ = Task.Run(() =>
        {
            try
            {
                var context = opts.ToPromptContext();
                var prompt = FormatPrompt(text);

                _model.Prompt(prompt, context, responseCallback: e =>
                {
                    if (e.IsError)
                    {
                        result.Success = false;
                        result.ErrorMessage = e.Response;
                        return false;
                    }
                    result.Append(e.Response);
                    return true;
                }, cancellationToken: cancellationToken);
            }
            finally
            {
                result.Complete();
            }
        }, CancellationToken.None);

        return Task.FromResult((ITextPredictionStreamingResult)result);
    }

    protected virtual void Dispose(bool disposing)
    {
        if (disposing)
        {
            _model.Dispose();
        }
    }

    public void Dispose()
    {
        Dispose(true);
        GC.SuppressFinalize(this);
    }
}
