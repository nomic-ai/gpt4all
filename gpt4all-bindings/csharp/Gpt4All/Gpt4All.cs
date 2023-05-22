using Gpt4All.Bindings;
using Gpt4All.Extensions;

namespace Gpt4All;

public class Gpt4All : IGpt4AllModel
{
    private readonly ILLModel _model;

    internal Gpt4All(ILLModel model)
    {
        _model = model;
    }

    public Task<ITextPredictionResult> GetPredictionAsync(string text, PredictRequestOptions opts, CancellationToken cancellationToken = default)
    {
        return Task.Run(() =>
        {
            var result = new TextPredictionResult();
            var context = opts.ToPromptContext();

            _model.Prompt(text, context, responseCallback: e =>
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
        var result = new TextPredictionStreamingResult();

        _ = Task.Run(() =>
        {
            try
            {
                var context = opts.ToPromptContext();

                _model.Prompt(text, context, responseCallback: e =>
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
