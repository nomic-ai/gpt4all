# GPT4All C# API

Native C# LLM bindings for all.


Use a chat session to keep context between completions. This is useful for efficient back and forth conversations.

### Chat

```cs
using GPt4All

var modelPath = "C:\\Path\\To\\Models\\orca-mini-3b-gguf2-q4_0.gguf";
using var model = new Gpt4All.LLModel(modelPath);
var response = model.Generate("What is 1 + 1?", "You are an advanced mathematician.");
Console.WriteLine(response.Response);
```

### Embedding

```cs
using GPt4All

var modelPath = "C:\\Path\\To\\Models\\nomic-embed-text-v1.f16.gguf";
using var model = new Gpt4All.LLModel(modelPath);
var response = model.Embed(["Hello, how are you? My name is Justin"], 768);
Console.WriteLine(string.Join(",", response));
```

### Development (Windows)

1. **Vulkan SDK**

    Building GPT4All requires the complete Vulkan SDK. You may download it from here: https://vulkan.lunarg.com/sdk/home

2. **Install Dependencies**

    Install [MinGW64](https://www.mingw-w64.org/) and CMAKE

    ```ps
    choco install mingw
    $env:Path += ";C:\ProgramData\mingw64\mingw64\bin"
    choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System'
    ``` 

    OR install Visual Studio 2022 and run the build script using `x64 Native Tools Command Prompt for VS 2022` (`vcvars64.bat`)

3. **Checkout the repo**

    ```ps
    git clone --recurse-submodules https://github.com/nomic-ai/gpt4all
    ```

4. **Run the `build_msvc.bat` build script**

    ```ps
    cd .\gpt4all\gpt4all-bindings\csharp
    .\build_msvc.bat
    ```

5. Run the example project

    ```ps
     cd .\Gpt4All.Examples\
    dotnet run
    ```