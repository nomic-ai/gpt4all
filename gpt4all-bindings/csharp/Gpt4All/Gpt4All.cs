using System.Diagnostics;
using System.Runtime.CompilerServices;
using Gpt4All.Bindings;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;

[assembly: InternalsVisibleTo("Gpt4All.Tests")]

namespace Gpt4All;

public class Gpt4All : IGpt4AllModel
{
    private readonly ILLModel _model;
    private readonly ILogger _logger;

    private const string ResponseErrorMessage =
        "The model reported an error during token generation error={ResponseError}";

    /// <inheritdoc/>
    public IPromptFormatter? PromptFormatter { get; set; }

    internal Gpt4All(ILLModel model, ILogger? logger = null)
    {
        _model = model;
        _logger = logger ?? NullLogger.Instance;
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
            _logger.LogInformation("Start prediction task");

            var sw = Stopwatch.StartNew();
            var result = new TextPredictionResult();
            var context = opts.ToPromptContext();
            var prompt = FormatPrompt(text);

            try
            {
                _model.Prompt(prompt, context, responseCallback: e =>
                {
                    if (e.IsError)
                    {
                        _logger.LogWarning(ResponseErrorMessage, e.Response);
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
                _logger.LogError(e, "Prompt error");
                result.Success = false;
            }

            sw.Stop();
            _logger.LogInformation("Prediction task completed elapsed={Elapsed}s", sw.Elapsed.TotalSeconds);

            return (ITextPredictionResult)result;
        }, CancellationToken.None);
    }

    public Task<ITextPredictionStreamingResult> GetStreamingPredictionAsync(string text, PredictRequestOptions opts, CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(text);

        var result = new TextPredictionStreamingResult();

        _ = Task.Run(() =>
        {
            _logger.LogInformation("Start streaming prediction task");
            var sw = Stopwatch.StartNew();

            try
            {
                var context = opts.ToPromptContext();
                var prompt = FormatPrompt(text);

                _model.Prompt(prompt, context, responseCallback: e =>
                {
                    if (e.IsError)
                    {
                        _logger.LogWarning(ResponseErrorMessage, e.Response);
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
                _logger.LogError(e, "Prompt error");
                result.Success = false;
            }
            finally
            {
                result.Complete();
                sw.Stop();
                _logger.LogInformation("Prediction task completed elapsed={Elapsed}s", sw.Elapsed.TotalSeconds);
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
