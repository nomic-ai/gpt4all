using Xunit;

namespace Gpt4All.Tests;

public class ModelFactoryTests
{
    private readonly Gpt4AllModelFactory _modelFactory;

    public ModelFactoryTests()
    {
        _modelFactory = new Gpt4AllModelFactory();
    }

    [Fact]
    [Trait(Traits.SkipOnCI, "True")]
    public void CanLoadLlamaModel()
    {
        using var model = _modelFactory.LoadModel(Constants.LLAMA_MODEL_PATH);
    }

    [Fact]
    [Trait(Traits.SkipOnCI, "True")]
    public void CanLoadGptjModel()
    {
        using var model = _modelFactory.LoadModel(Constants.GPTJ_MODEL_PATH);
    }

    [Fact]
    [Trait(Traits.SkipOnCI, "True")]
    public void CanLoadMptModel()
    {
        using var model = _modelFactory.LoadModel(Constants.MPT_MODEL_PATH);
    }
}
