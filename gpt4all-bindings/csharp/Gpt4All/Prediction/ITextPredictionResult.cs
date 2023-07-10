namespace Gpt4All;

public interface ITextPredictionResult
{
    bool Success { get; }

    string? ErrorMessage { get; }

    Task<string> GetPredictionAsync(CancellationToken cancellationToken = default);
}
