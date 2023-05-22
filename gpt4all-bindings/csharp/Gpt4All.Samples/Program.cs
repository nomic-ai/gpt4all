using Gpt4All;

var modelFactory = new Gpt4AllModelFactory();

var modelPath = args[0];

using var model = modelFactory.LoadModel(modelPath);

var input = args.Length > 1 ? args[1] : "Name 3 colors.";

var result = await model.GetStreamingPredictionAsync(
    input,
    PredictRequestOptions.Defaults);

await foreach (var token in result.GetPredictionStreamingAsync())
{
    Console.Write(token);
}

Console.WriteLine();
Console.WriteLine("DONE.");
