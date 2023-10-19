# C# GPT4All

This package contains a set of C# bindings around the `llmodel` C-API.

## Documentation
TBD

## Installation
TBD NuGet

## Project Structure
```
gpt4all-bindings/
└── csharp                
    ├── Gpt4All               // .NET Bindigs
    ├── Gpt4All.Samples       // Sample project
    ├── build_win-msvc.ps1    // Native build scripts
    ├── build_win-mingw.ps1   
    ├── build_linux.sh        
    └── runtimes              // [POST-BUILD] Platform-specific native libraries
          ├── win-x64
          ├── ...
          └── linux-x64
```

## Prerequisites

On Windows and Linux, building GPT4All requires the complete Vulkan SDK. You may download it from here: https://vulkan.lunarg.com/sdk/home

macOS users do not need Vulkan, as GPT4All will use Metal instead.

## Local Build Instructions
> **Note** 
> Tested On:
>  - Windows 11 22H + VS2022 (CE) x64
>  - Linux Ubuntu x64
>  - Linux Ubuntu (WSL2) x64

1. Setup the repository
2. Build the native libraries for the platform of choice (see below)
3. Build the C# Bindings (NET6+ SDK is required)
```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all
cd gpt4all/gpt4all-bindings/csharp
```
### Linux
1. Setup build environment and install NET6+ SDK with the appropriate procedure for your distribution
```
sudo apt-get update
sudo apt-get install -y cmake build-essential
chmod +x ./build_linux.sh
```
2. `./build_linux.sh`
3. The native libraries should be present at `.\native\linux-x64`

### Windows - MinGW64
#### Additional requirements
  - [MinGW64](https://www.mingw-w64.org/) 
  - CMAKE
1. Setup
```
choco install mingw
$env:Path += ";C:\ProgramData\mingw64\mingw64\bin"
choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System'
```
2. Run the `./build_win-mingw.ps1` build script
3. The native libraries should be present at `.\native\win-x64`

### Windows - MSVC
#### Additional requirements
  - Visual Studio 2022
1. Open a terminal using the  `x64 Native Tools Command Prompt for VS 2022` (`vcvars64.bat`)
2. Run the `./build_win-msvc.ps1` build script
3. `libllmodel.dll` and `libllama.dll` should be present at `.\native\win-x64`

> **Warning** 
> If the build fails with: '**error C7555: use of designated initializers requires at least '/std:c++20'**'
>
> Modify `cd gpt4all/gpt4all-backends/CMakeLists.txt` adding `CXX_STANDARD_20` to `llmodel` properties.
> ```cmake
> set_target_properties(llmodel PROPERTIES
>                              VERSION ${PROJECT_VERSION}
>                              CXX_STANDARD 20 # <---- ADD THIS -----------------------
>                              SOVERSION ${PROJECT_VERSION_MAJOR})
> ```
## C# Bindings Build Instructions
Build the `Gpt4All` (or `Gpt4All.Samples`) projects from within VisualStudio.
### Try the bindings
```csharp
using Gpt4All;

// load the model
var modelFactory = new ModelFactory();

using var model = modelFactory.LoadModel("./path/to/ggml-gpt4all-j-v1.3-groovy.bin");

var input = "Name 3 Colors";

// request a prediction
var result = await model.GetStreamingPredictionAsync(
    input, 
    PredictRequestOptions.Defaults);

// asynchronously print the tokens as soon as they are produces by the model
await foreach(var token in result.GetPredictionStreamingAsync())
{
    Console.Write(token);
}
```
Output:
```
gptj_model_load: loading model from 'ggml-gpt4all-j-v1.3-groovy.bin' - please wait ...
gptj_model_load: n_vocab = 50400
[...TRUNCATED...]
gptj_model_load: ggml ctx size = 5401.45 MB
gptj_model_load: kv self size  =  896.00 MB
gptj_model_load: ................................... done
gptj_model_load: model size =  3609.38 MB / num tensors = 285

Black, Blue and White
```
