using System.Diagnostics;
using Microsoft.Extensions.Logging;
using Gpt4All.Bindings;
using Microsoft.Extensions.Logging.Abstractions;

namespace Gpt4All;

public class Gpt4AllModelFactory : IGpt4AllModelFactory
{
    private readonly ILoggerFactory _loggerFactory;
    private readonly ILogger _logger;

    public Gpt4AllModelFactory(ILoggerFactory? loggerFactory = null)
    {
        _loggerFactory = loggerFactory ?? NullLoggerFactory.Instance;
        _logger = _loggerFactory.CreateLogger<Gpt4AllModelFactory>();
    }

    private IGpt4AllModel CreateModel(string modelPath, ModelType? modelType = null)
    {
        var modelType_ = modelType ?? ModelFileUtils.GetModelTypeFromModelFileHeader(modelPath);

        _logger.LogInformation("Creating model path={ModelPath} type={ModelType}", modelPath, modelType_);

        var handle = modelType_ switch
        {
            ModelType.LLAMA => NativeMethods.llmodel_llama_create(),
            ModelType.GPTJ => NativeMethods.llmodel_gptj_create(),
            ModelType.MPT => NativeMethods.llmodel_mpt_create(),
            _ => NativeMethods.llmodel_model_create(modelPath),
        };

        _logger.LogDebug("Model created handle=0x{ModelHandle:X8}", handle);
        _logger.LogInformation("Model loading started");

        var loadedSuccessfully = NativeMethods.llmodel_loadModel(handle, modelPath);

        _logger.LogInformation("Model loading completed success={ModelLoadSuccess}", loadedSuccessfully);

        if (loadedSuccessfully == false)
        {
            throw new Exception($"Failed to load model: '{modelPath}'");
        }

        var logger = _loggerFactory.CreateLogger<LLModel>();

        var underlyingModel = LLModel.Create(handle, modelType_, logger: logger);

        Debug.Assert(underlyingModel.IsLoaded());

        return new Gpt4All(underlyingModel, logger: logger);
    }

    public IGpt4AllModel LoadModel(string modelPath) => CreateModel(modelPath, modelType: null);

    public IGpt4AllModel LoadMptModel(string modelPath) => CreateModel(modelPath, ModelType.MPT);

    public IGpt4AllModel LoadGptjModel(string modelPath) => CreateModel(modelPath, ModelType.GPTJ);

    public IGpt4AllModel LoadLlamaModel(string modelPath) => CreateModel(modelPath, ModelType.LLAMA);
}
