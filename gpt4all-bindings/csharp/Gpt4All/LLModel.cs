using System.Text;
using Gpt4All.Models;
using Microsoft.Extensions.Logging;

namespace Gpt4All;

public class LLModel : IDisposable
{
    private readonly string _model;
    private readonly INativeInteropWrapper _interopWrapper;
    private readonly ILogger _logger;

    private IntPtr _modelPtr;
    private bool _loaded;

    internal LLModel(string model, INativeInteropWrapper interopWrapper, ILogger logger)
    {
        _model = model;
        _logger = logger;
        _interopWrapper = interopWrapper;
    }

    public void Dispose()
    {
        GC.SuppressFinalize(this);
        DestroyModel();
    }

    public unsafe EmbedResponse Embed(
        IEnumerable<string> stringToEmbed,
        int dimensionality = 768,
        string? prefix = null,
        bool doMean = false,
        bool atlas = false,
        CancellationToken cancelToken = default
    )
    {
        LoadModel();

        try
        {
            var texts = stringToEmbed.ToArray();
            long tokenCount;

            var embeddings = _interopWrapper.Embed(
                _modelPtr,
                texts,
                prefix,
                dimensionality,
                doMean,
                atlas,
                (batchSizes, numberOfBatches, backend) =>
                {
                    return !cancelToken.IsCancellationRequested;
                },
                out tokenCount);

            return new EmbedResponse
            {
                Embeddings = embeddings,
                TokenCount = tokenCount
            };
        }
        catch (Exception e)
        {
            _logger.LogError(e, "Error embedding strings");
            throw;
        }
    }

    public GenerateResponse Generate(
        string prompt,
        string promptTemplate,
        PromptContext? context = null,
        CancellationToken cancelToken = default)
    {
        LoadModel();

        try
        {
            // Map from context
            var totalTokens = 0;
            var stringBuilder = new StringBuilder();
            var useContext = context ?? new();

            var newContext = _interopWrapper.Prompt(
                _modelPtr,
                prompt,
                promptTemplate,
                (tokenId) =>
                {
                    totalTokens++;
                    return !cancelToken.IsCancellationRequested;
                },
                (tokenId, response) =>
                {
                    totalTokens++;
                    stringBuilder.Append(response);
                    return !cancelToken.IsCancellationRequested;
                },
                (isRecalculating) =>
                {
                    return !cancelToken.IsCancellationRequested;
                },
                useContext,
                false,
                null
            );

            return new GenerateResponse
            {
                PromptContext = newContext,
                Response = stringBuilder.ToString(),
                TotalTokens = totalTokens
            };
        }
        catch (Exception e)
        {
            _logger.LogError(e, "Error generating outputs");
            throw;
        }
    }

    private unsafe void DestroyModel()
    {
        if (_loaded && _modelPtr != IntPtr.Zero)
        {
            _interopWrapper.ModelDestroy(_modelPtr);
            _modelPtr = IntPtr.Zero;
            _loaded = false;
            _logger.LogDebug("Model destroyed");
        }
        else
        {
            _logger.LogDebug("Model already destroyed or never loaded");
        }
    }

    private unsafe void LoadModel(int maxContextSize = 2048, int numGpuLayers = 100)
    {
        if (_loaded)
        {
            return;
        }

        var handle = CreateModel();
        var loaded = _interopWrapper.LoadModel(handle, _model, maxContextSize, numGpuLayers);
        if (!loaded)
        {
            _logger.LogError("Error loading model");
            throw new Exception("Error loading model");
        }

        _logger.LogDebug("Model loaded successfully");
        _loaded = true;
    }

    private unsafe IntPtr CreateModel(string buildVariant = "auto")
    {
        // Ensure model exists
        if (!File.Exists(_model))
        {
            _logger.LogError("Model file not found: {Model}", _model);
            throw new FileNotFoundException("Model file not found", _model);
        }

        // Set library path
        var pathToLibrary = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location)!;
        _interopWrapper.SetImplementationSearchPath(pathToLibrary);

        try
        {
            // Create the model
            _modelPtr = _interopWrapper.ModelCreate(_model, buildVariant);
            _logger.LogInformation("Model created successfully");
            return _modelPtr;
        }
        catch (Exception e)
        {
            _logger.LogError(e, "Error creating model");
            throw;
        }
    }
}
