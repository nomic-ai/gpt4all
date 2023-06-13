using System.IO;
using Xunit;

namespace Gpt4All.Tests;

public class ModelFactoryTests
{
    private readonly Gpt4AllModelFactory _modelFactory;

    public ModelFactoryTests()
    {
        _modelFactory = new Gpt4AllModelFactory();
    }


    [Theory]
    [InlineData(Constants.GPTJ_MODEL_PATH)]
    [InlineData(Constants.MPT_MODEL_PATH)]
    [InlineData(Constants.LLAMA_MODEL_PATH)]
    public void CanLoadModel(string modelFilename)
    {
        var modelPath = Path.Join(Constants.MODELS_BASE_DIR, modelFilename);
        using var model = _modelFactory.LoadModel(modelPath);
    }
}
