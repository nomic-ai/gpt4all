namespace Gpt4All;

public interface ITextPredictionStreamingResult : ITextPredictionResult
{
    IAsyncEnumerable<string> GetPredictionStreamingAsync(CancellationToken cancellationToken = default);
}
