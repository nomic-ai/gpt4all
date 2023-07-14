# GPT4All C# API
The `GPT4All` .NET C# library provides bindings to gpt4all C/C++ model backend libraries.
The source code and local build instructions can be found [here](https://github.com/nomic-ai/gpt4all/tree/main/gpt4all-bindings/csharp).


## Installation

!!! info "NuGet"
	The package will be soon distribute on **NuGet**. Until that moment please: 
	
	  1. Follow the local build instructions to build the backend
	  2. Reference the `Gpt4All` project from source

=== ".NET CLI"
	```bash
	dotnet add package Gpt4All
	```
=== "Package Manager"
	```
	PM> NuGet\Install-Package Gpt4All
	```
=== "Package Reference"
    ```xml
	<PackageReference Include="Gpt4All" Version="0.8.0-alpha" />
	```
	
## Text Generation

=== "GPT4All Example"
    ```csharp
	using Gpt4All;
	
	var modelFactory = new Gpt4AllModelFactory();
	
	using var model = modelFactory.LoadModel("./orca-mini-3b.ggmlv3.q4_0.bin");

	model.Context.TokensToPredict = 3;

	var result = await model.GetStreamingPredictionAsync("The capital of France is ");

	await foreach (var token in result.GetPredictionStreamingAsync())
	{
		Console.Write(token);
	}
    ```
=== "Output"
    ```
    1. Paris
    ```

## Logging

!!! note
	The following packages are required for this example:
	 
	  - `Serilog.Sinks.Console`
	  - `Serilog.Extensions.Logging`

=== "GPT4All Example"
    ```csharp
	using Gpt4All;
	using Serilog;
	using Serilog.Core;
	using Microsoft.Extensions.Logging;

	var logger = new LoggerConfiguration()
		.WriteTo.Console()
		.MinimumLevel.Debug()
		.CreateLogger();

	var loggerFactory = new LoggerFactory().AddSerilog(logger);

	var modelFactory = new Gpt4AllModelFactory(loggerFactory: loggerFactory);

	using var model = modelFactory.LoadModel("./orca-mini-3b.ggmlv3.q4_0.bin");

	model.Context.TokensToPredict = 3;

	var result = await model.GetStreamingPredictionAsync("The capital of France is ");
    ```
=== "Output"
    ```
    [22:51:00 INF] Creating model path=/models/orca-mini-3b.ggmlv3.q4_0.bin type=LLAMA
	[22:51:00 DBG] Model created handle=0x1762702781760
	[22:51:00 INF] Model loading started
	[22:51:02 INF] Model loading completed success=True
	[22:51:02 INF] Start streaming prediction task
	[22:51:02 INF] Prompt input='The capital of France is' ctx=
			{
				logits_size = 0
				tokens_size = 0
				n_past = 0
				n_ctx = 1024
				n_predict = 128
				[...truncated for brevity..]
			}
	[22:51:10 INF] Prediction task completed elapsed=7.8376754s
    ```

## Using in ASP NET Core
If you are using the `Gpt4All` package in an ASP .NET Core application and you are encountering the *"Model format not supported (no matching implementation found)"* error, 
the issue may be caused by the Current Working Directory not being set to the path of the executable that started the process. 
To fix the problem try to set the implementations search path where the native libraries reside (where the .exe is located usually) **before loading the model**.
For example:

```csharp
LLModel.SetImplementationSearchPath(Path.GetDirectoryName(Environment.ProcessPath)!);
```