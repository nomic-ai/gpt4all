using System.IO;
using System.Text;
using System.Threading.Tasks;
using Xunit;
using Xunit.Abstractions;

namespace Gpt4All.Tests
{
    public class ModelTests
    {
        private readonly ITestOutputHelper _output;
        private readonly Gpt4AllModelFactory _modelFactory;

        public ModelTests(ITestOutputHelper output)
        {
            _output = output;
            _modelFactory = new Gpt4AllModelFactory();
        }

        [Theory]
        [InlineData(Constants.GPTJ_MODEL_PATH)]
        [InlineData(Constants.MPT_MODEL_PATH)]
        [InlineData(Constants.LLAMA_MODEL_PATH)]
        public async Task ModelReturnsPrediction(string modelFilename)
        {
            var modelPath = Path.Join(Constants.MODELS_BASE_DIR, modelFilename);
            using var model = _modelFactory.LoadModel(modelPath);

            var result = await model.GetStreamingPredictionAsync(
                "Good morning, how are you today?",
                new PredictRequestOptions() { TokensToPredict = 10 });

            var sb = new StringBuilder();
            await foreach (var token in result.GetPredictionStreamingAsync())
            {
                _output.WriteLine("Generated token: " + token);
                sb.Append(token);
            }

            var returnedPrediction = sb.ToString();
            _output.WriteLine(returnedPrediction);
            Assert.NotEmpty(returnedPrediction);
        }
    }
}