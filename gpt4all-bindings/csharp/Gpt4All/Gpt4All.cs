using System.Diagnostics;
using Gpt4All.Bindings;
using Gpt4All.Extensions;
using Microsoft.Extensions.Logging;

namespace Gpt4All;

public class Gpt4All : IGpt4AllModel
{
    private readonly ILLModel _model;
    private readonly ILogger? _logger;

    private const string ResponseErrorMessage =
        "The model reported an error during token generation error={ResponseError}";

    internal Gpt4All(ILLModel model, ILogger? logger = null)
    {
        _model = model;
        _logger = logger;
    }

    public Task<ITextPredictionResult> GetPredictionAsync(string text, PredictRequestOptions opts, CancellationToken cancellationToken = default)
    {
        return Task.Run(() =>
        {
            _logger?.LogInformation("Start prediction task");

            var sw = Stopwatch.StartNew();
            var result = new TextPredictionResult();
            var context = opts.ToPromptContext();

            try
            {
                _model.Prompt(text, context, responseCallback: e =>
                {
                    if (e.IsError)
                    {
                        _logger?.LogWarning(ResponseErrorMessage, e.Response);
                        result.Success = false;
                        result.ErrorMessage = e.Response;
                        return false;
                    }
                    result.Append(e.Response);
                    return true;
                }, cancellationToken: cancellationToken);
            }
            catch (Exception e)
            {
                _logger?.LogError(e, "Prompt error");
                result.Success = false;
            }

            sw.Stop();
            _logger?.LogInformation("Prediction task completed elapsed={Elapsed}s", sw.Elapsed.TotalSeconds);

            return (ITextPredictionResult)result;
        }, CancellationToken.None);
    }

    public Task<ITextPredictionStreamingResult> GetStreamingPredictionAsync(string text, PredictRequestOptions opts, CancellationToken cancellationToken = default)
    {
        var result = new TextPredictionStreamingResult();

        _ = Task.Run(() =>
        {
            _logger?.LogInformation("Start streaming prediction task");
            var sw = Stopwatch.StartNew();

            try
            {
                var context = opts.ToPromptContext();

                _model.Prompt(text, context, responseCallback: e =>
                {
                    if (e.IsError)
                    {
                        _logger?.LogWarning(ResponseErrorMessage, e.Response);
                        result.Success = false;
                        result.ErrorMessage = e.Response;
                        return false;
                    }
                    result.Append(e.Response);
                    return true;
                }, cancellationToken: cancellationToken);
            }
            catch (Exception e)
            {
                _logger?.LogError(e, "Prompt error");
                result.Success = false;
            }
            finally
            {
                result.Complete();
                sw.Stop();
                _logger?.LogInformation("Prediction task completed elapsed={Elapsed}s", sw.Elapsed.TotalSeconds);
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
