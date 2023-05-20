namespace Gpt4All;

public interface ITextPrediction
{
    Task<ITextPredictionResult> GetPredictionAsync(
        string text,
        PredictRequestOptions opts,
        CancellationToken cancellation = default);

    Task<ITextPredictionStreamingResult> GetStreamingPredictionAsync(
        string text,
        PredictRequestOptions opts,
        CancellationToken cancellationToken = default);
}