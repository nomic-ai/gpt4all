namespace Gpt4All.Bindings;

/// <summary>
/// Represents the interface exposed by the universal wrapper for GPT4All language models built around llmodel C-API.
/// </summary>
public interface ILLModel : IDisposable
{
    ulong GetStateSizeBytes();

    int GetThreadCount();

    void SetThreadCount(int threadCount);

    bool IsLoaded();

    bool Load(string modelPath);

    void Prompt(
        string text,
        LLModelPromptContext context,
        Func<ModelPromptEventArgs, bool>? promptCallback = null,
        Func<ModelResponseEventArgs, bool>? responseCallback = null,
        Func<ModelRecalculatingEventArgs, bool>? recalculateCallback = null,
        CancellationToken cancellationToken = default);

    unsafe ulong RestoreStateData(byte* destination);

    unsafe ulong SaveStateData(byte* source);
}
