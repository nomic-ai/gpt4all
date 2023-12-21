using System.Diagnostics;
using Microsoft.Extensions.Logging.Abstractions;
using Microsoft.Extensions.Logging;
using Gpt4All.Bindings;
using Gpt4All.LibraryLoader;

namespace Gpt4All;

public class Gpt4AllModelFactory : IGpt4AllModelFactory
{
    private readonly ILoggerFactory _loggerFactory;
    private readonly ILogger _logger;
    private static bool bypassLoading;
    private static string? libraryPath;

    private static readonly Lazy<LoadResult> libraryLoaded = new(() =>
    {
        return NativeLibraryLoader.LoadNativeLibrary(Gpt4AllModelFactory.libraryPath, Gpt4AllModelFactory.bypassLoading);
    }, true);

    public Gpt4AllModelFactory(string? libraryPath = default, bool bypassLoading = true, ILoggerFactory? loggerFactory = null)
    {
        _loggerFactory = loggerFactory ?? NullLoggerFactory.Instance;
        _logger = _loggerFactory.CreateLogger<Gpt4AllModelFactory>();
        Gpt4AllModelFactory.libraryPath = libraryPath;
        Gpt4AllModelFactory.bypassLoading = bypassLoading;

        if (!libraryLoaded.Value.IsSuccess)
        {
            throw new Exception($"Failed to load native gpt4all library. Error: {libraryLoaded.Value.ErrorMessage}");
        }
    }

    private IGpt4AllModel CreateModel(string modelPath)
    {
        var modelType_ = ModelFileUtils.GetModelTypeFromModelFileHeader(modelPath);
        _logger.LogInformation("Creating model path={ModelPath} type={ModelType}", modelPath, modelType_);
        IntPtr error;
        var handle = NativeMethods.llmodel_model_create2(modelPath, "auto", out error);
        _logger.LogDebug("Model created handle=0x{ModelHandle:X8}", handle);
        _logger.LogInformation("Model loading started");
        var loadedSuccessfully = NativeMethods.llmodel_loadModel(handle, modelPath, 2048);
        _logger.LogInformation("Model loading completed success={ModelLoadSuccess}", loadedSuccessfully);
        if (!loadedSuccessfully)
        {
            throw new Exception($"Failed to load model: '{modelPath}'");
        }

        var logger = _loggerFactory.CreateLogger<LLModel>();
        var underlyingModel = LLModel.Create(handle, modelType_, logger: logger);

        Debug.Assert(underlyingModel.IsLoaded());

        return new Gpt4All(underlyingModel, logger: logger);
    }

    public IGpt4AllModel LoadModel(string modelPath) => CreateModel(modelPath);
}
