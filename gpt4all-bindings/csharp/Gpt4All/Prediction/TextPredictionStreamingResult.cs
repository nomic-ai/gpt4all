using System.Text;
using System.Threading.Channels;

namespace Gpt4All;

public record TextPredictionStreamingResult : ITextPredictionStreamingResult
{
    private readonly Channel<string> _channel;

    public bool Success { get; internal set; } = true;

    public string? ErrorMessage { get; internal set; }

    public Task Completion => _channel.Reader.Completion;

    internal TextPredictionStreamingResult()
    {
        _channel = Channel.CreateUnbounded<string>();
    }

    internal bool Append(string token)
    {
        return _channel.Writer.TryWrite(token);
    }

    internal void Complete()
    {
        _channel.Writer.Complete();
    }

    public async Task<string> GetPredictionAsync(CancellationToken cancellationToken = default)
    {
        var sb = new StringBuilder();

        var tokens = GetPredictionStreamingAsync(cancellationToken).ConfigureAwait(false);

        await foreach (var token in tokens)
        {
            sb.Append(token);
        }

        return sb.ToString();
    }

    public IAsyncEnumerable<string> GetPredictionStreamingAsync(CancellationToken cancellationToken = default)
    {
        return _channel.Reader.ReadAllAsync(cancellationToken);
    }
}
