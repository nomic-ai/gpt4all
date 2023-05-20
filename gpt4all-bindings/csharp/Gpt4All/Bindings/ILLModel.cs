namespace Gpt4All.Bindings;

public interface ILLModel : IDisposable
{
    ModelType ModelType { get; }

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

    unsafe void RestoreStateData(byte* destination);

    unsafe void SaveStateData(byte* source);

}