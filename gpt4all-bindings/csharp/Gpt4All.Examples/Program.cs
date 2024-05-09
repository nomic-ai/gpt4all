using Microsoft.Extensions.Logging;

using var loggerFactory = LoggerFactory.Create(loggingBuilder => loggingBuilder
    .SetMinimumLevel(LogLevel.Trace)
    .AddConsole());

var logger = loggerFactory.CreateLogger<Program>();
//var modelPath = "C:\\Users\\jrobb\\source\\temp\\gpt4all\\gpt4all-bindings\\csharp\\models\\orca-mini-3b-gguf2-q4_0.gguf";
//using var model = new Gpt4All.LLModel(modelPath, logger);
//var response = model.Generate("Hello, how are you? My name is Justin", "You are a helpful AI Assistant");
//Console.WriteLine("RESPONSE:");
//Console.WriteLine(response.Response);

//response = model.Generate("What was my name?", "You are a helpful AI Assistant", response.PromptContext);
//Console.WriteLine("RESPONSE:");
//Console.WriteLine(response.Response);

var modelPath = "C:\\Users\\jrobb\\source\\personal\\content-embedder\\models\\nomic-embed-text-v1.f16.gguf";
using var model = new Gpt4All.LLModel(modelPath, logger);
var response = model.Embed(["Hello, how are you? My name is Justin"], 768);
Console.WriteLine("RESPONSE:");
Console.WriteLine(string.Join(",", response));
