using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;

namespace Gpt4All.Bindings;

/// <summary>
/// Arguments for the response processing callback
/// </summary>
/// <param name="TokenId">The token id of the response</param>
/// <param name="Response"> The response string. NOTE: a token_id of -1 indicates the string is an error string</param>
/// <return>
/// A bool indicating whether the model should keep generating
/// </return>
public record ModelResponseEventArgs(int TokenId, string Response)
{
    public bool IsError => TokenId == -1;
}

/// <summary>
/// Arguments for the prompt processing callback
/// </summary>
/// <param name="TokenId">The token id of the prompt</param>
/// <return>
/// A bool indicating whether the model should keep processing
/// </return>
public record ModelPromptEventArgs(int TokenId)
{
}

/// <summary>
/// Arguments for the recalculating callback
/// </summary>
/// <param name="IsRecalculating"> whether the model is recalculating the context.</param>
/// <return>
/// A bool indicating whether the model should keep generating
/// </return>
public record ModelRecalculatingEventArgs(bool IsRecalculating);

/// <summary>
/// Base class and universal wrapper for GPT4All language models built around llmodel C-API.
/// </summary>
public class LLModel : ILLModel
{
    protected readonly IntPtr _handle;
    private readonly ILogger _logger;
    private bool _disposed;

    internal LLModel(IntPtr handle, ILogger? logger = null)
    {
        _handle = handle;
        _logger = logger ?? NullLogger.Instance;
    }

    /// <summary>
    /// Create a new model from a pointer
    /// </summary>
    /// <param name="handle">Pointer to underlying model</param>
    public static LLModel Create(IntPtr handle, ILogger? logger = null)
    {
        return new LLModel(handle, logger: logger);
    }

    /// <summary>
    /// Generate a response using the model
    /// </summary>
    /// <param name="text">The input promp</param>
    /// <param name="context">The context</param>
    /// <param name="promptCallback">A callback function for handling the processing of prompt</param>
    /// <param name="responseCallback">A callback function for handling the generated response</param>
    /// <param name="recalculateCallback">A callback function for handling recalculation requests</param>
    /// <param name="cancellationToken"></param>
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

        _logger.LogInformation("Prompt input='{Prompt}' ctx={Context}", text, context.Dump());

        NativeMethods.llmodel_prompt(
            _handle,
            text,
            (tokenId) =>
            {
                if (cancellationToken.IsCancellationRequested) return false;
                if (promptCallback == null) return true;
                var args = new ModelPromptEventArgs(tokenId);
                return promptCallback(args);
            },
            (tokenId, response) =>
            {
                if (cancellationToken.IsCancellationRequested)
                {
                    _logger.LogDebug("ResponseCallback evt=CancellationRequested");
                    return false;
                }

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

    /// <summary>
    ///  Set the number of threads to be used by the model.
    /// </summary>
    /// <param name="threadCount">The new thread count</param>
    public void SetThreadCount(int threadCount)
    {
        NativeMethods.llmodel_setThreadCount(_handle, threadCount);
    }

    /// <summary>
    /// Get  the number of threads used by the model.
    /// </summary>
    /// <returns>the number of threads used by the model</returns>
    public int GetThreadCount()
    {
        return NativeMethods.llmodel_threadCount(_handle);
    }

    /// <summary>
    /// Get the size of the internal state of the model.
    /// </summary>
    /// <remarks>
    /// This state data is specific to the type of model you have created.
    /// </remarks>
    /// <returns>the size in bytes of the internal state of the model</returns>
    public ulong GetStateSizeBytes()
    {
        return NativeMethods.llmodel_get_state_size(_handle);
    }

    /// <summary>
    /// Saves the internal state of the model to the specified destination address.
    /// </summary>
    /// <param name="source">A pointer to the src</param>
    /// <returns>The number of bytes copied</returns>
    public unsafe ulong SaveStateData(byte* source)
    {
        return NativeMethods.llmodel_save_state_data(_handle, source);
    }

    /// <summary>
    /// Restores the internal state of the model using data from the specified address.
    /// </summary>
    /// <param name="destination">A pointer to destination</param>
    /// <returns>the number of bytes read</returns>
    public unsafe ulong RestoreStateData(byte* destination)
    {
        return NativeMethods.llmodel_restore_state_data(_handle, destination);
    }

    /// <summary>
    /// Check if the model is loaded.
    /// </summary>
    /// <returns>true if the model was loaded successfully, false otherwise.</returns>
    public bool IsLoaded()
    {
        return NativeMethods.llmodel_isModelLoaded(_handle);
    }

    /// <summary>
    /// Load the model from a file.
    /// </summary>
    /// <param name="modelPath">The path to the model file.</param>
    /// <returns>true if the model was loaded successfully, false otherwise.</returns>
    public bool Load(string modelPath)
    {
        return NativeMethods.llmodel_loadModel(_handle, modelPath, 2048, 100);
    }

    protected void Destroy()
    {
        NativeMethods.llmodel_model_destroy(_handle);
    }
    protected virtual void Dispose(bool disposing)
    {
        if (_disposed) return;

        if (disposing)
        {
            // dispose managed state
        }

        Destroy();

        _disposed = true;
    }

    public void Dispose()
    {
        Dispose(disposing: true);
        GC.SuppressFinalize(this);
    }
}
