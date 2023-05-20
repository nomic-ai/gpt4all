namespace Gpt4All.Bindings;

public record ModelResponseEventArgs(int TokenId, string Response)
{
    public bool IsError => TokenId == -1;
}

public record ModelPromptEventArgs(int TokenId, string Prompt)
{
    public bool IsError => TokenId == -1;
}

public record ModelRecalculatingEventArgs(bool IsRecalculating);

/// <summary>
/// Represents a minimal wrapper around the native llmodel API
/// <remarks>
/// This class is NOT meant to be THREAD-SAFE.
/// If thread-safety is necessary, provide it at an higher level.
/// </remarks>
/// </summary>
public class LLModel : ILLModel
{
    protected readonly IntPtr _handle;
    private readonly ModelType _modelType;
    private bool _disposed;

    public ModelType ModelType => _modelType;

    internal LLModel(IntPtr handle, ModelType modelType)
    {
        _handle = handle;
        _modelType = modelType;
    }

    public static LLModel Create(IntPtr handle, ModelType modelType)
    {
        return new LLModel(handle, modelType);
    }

    public void Prompt(
        string text,
        LLModelPromptContext context,
        Func<ModelPromptEventArgs, bool>? promptCallback = null,
        Func<ModelResponseEventArgs, bool>? responseCallback = null,
        Func<ModelRecalculatingEventArgs, bool>? recalculateCallback = null,
        CancellationToken cancellationToken = default)
    {
        GC.KeepAlive(promptCallback);
        GC.KeepAlive(responseCallback);
        GC.KeepAlive(recalculateCallback);
        GC.KeepAlive(cancellationToken);

        NativeMethods.llmodel_prompt(
            _handle,
            text,
            (tokenId, prompt) =>
            {
                if (cancellationToken.IsCancellationRequested) return false;
                if (promptCallback == null) return true;
                var args = new ModelPromptEventArgs(tokenId, prompt);
                return promptCallback(args);
            },
            (tokenId, response) =>
            {
                if (cancellationToken.IsCancellationRequested) return false;
                if (responseCallback == null) return true;
                var args = new ModelResponseEventArgs(tokenId, response);
                return responseCallback(args);
            },
            (isRecalculating) =>
            {
                if (cancellationToken.IsCancellationRequested) return false;
                if (recalculateCallback == null) return true;
                var args = new ModelRecalculatingEventArgs(isRecalculating);
                return recalculateCallback(args);
            },
            ref context.UnderlyingContext
        );
    }

    public void SetThreadCount(int threadCount)
    {
        NativeMethods.llmodel_setThreadCount(_handle, threadCount);
    }

    public int GetThreadCount()
    {
        return NativeMethods.llmodel_threadCount(_handle);
    }

    public ulong GetStateSizeBytes()
    {
        return NativeMethods.llmodel_get_state_size(_handle);
    }

    public unsafe void SaveStateData(byte* source)
    {
        NativeMethods.llmodel_save_state_data(_handle, source);
    }

    public unsafe void RestoreStateData(byte* destination)
    {
        NativeMethods.llmodel_restore_state_data(_handle, destination);
    }

    public bool IsLoaded()
    {
        return NativeMethods.llmodel_isModelLoaded(_handle);
    }

    public bool Load(string modelPath)
    {
        return NativeMethods.llmodel_loadModel(_handle, modelPath);
    }

    protected void Destroy()
    {
        NativeMethods.llmodel_model_destroy(_handle);
    }

    protected void DestroyLLama()
    {
        NativeMethods.llmodel_llama_destroy(_handle);
    }

    protected void DestroyGptj()
    {
        NativeMethods.llmodel_gptj_destroy(_handle);
    }

    protected void DestroyMtp()
    {
        NativeMethods.llmodel_mpt_destroy(_handle);
    }

    protected virtual void Dispose(bool disposing)
    {
        if (_disposed) return;

        if (disposing)
        {
            // dispose managed state
        }

        switch (_modelType)
        {
            case ModelType.LLAMA:
                DestroyLLama();
                break;
            case ModelType.GPTJ:
                DestroyGptj();
                break;
            case ModelType.MPT:
                DestroyMtp();
                break;
            default:
                Destroy();
                break;
        }

        _disposed = true;
    }

    public void Dispose()
    {
        Dispose(disposing: true);
        GC.SuppressFinalize(this);
    }
}