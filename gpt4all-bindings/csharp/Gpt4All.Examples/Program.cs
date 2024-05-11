using Gpt4All;

// Usage: dotnet run --project Gpt4All.Examples <operation> <model> 
// Example: dotnet run --project Gpt4All.Examples embed ".\models\nomic-embed-text-v1.f16.gguf"
if (args.Length < 3)
{
    Console.WriteLine("Usage: dotnet run --project Gpt4All.Examples <operation> <model> [options]");
    return;
}

var operation = args[0];
var modelPath = args[1];

if (operation == "embed")
{
    if (args.Length < 4)
    {
        Console.WriteLine("Usage: dotnet run --project Gpt4All.Examples embed <model> <input> <dims>");
        return;
    }

    var input = args[3];
    var dims = int.Parse(args[4]);

    using var embedModel = ModelFactory.CreateLLModel(modelPath);
    var embedResponse = embedModel.Embed(new[] { input }, dims);
    Console.WriteLine(string.Join(",", embedResponse.Embeddings));
}
if (operation == "prompt")
{
    if (args.Length < 4)
    {
        Console.WriteLine("Usage: dotnet run --project Gpt4All.Examples prompt <model> <prompt> <promptTemplate>");
        return;
    }

    var prompt = args[3];
    var promptTemplate = args[4];

    using var model = ModelFactory.CreateLLModel(modelPath);
    var response = model.Generate(prompt, promptTemplate);
    Console.WriteLine(response.Response);
}
else
{
    Console.WriteLine($"Unknown operation: {operation}. Must be one of embed, prompt");
}
