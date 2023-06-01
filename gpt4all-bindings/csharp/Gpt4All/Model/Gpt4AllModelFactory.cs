using Gpt4All.Bindings;
using Gpt4All.LibraryLoader;
using System.Diagnostics;

namespace Gpt4All;

public class Gpt4AllModelFactory : IGpt4AllModelFactory
{
    private static bool bypassLoading;
    private static string? libraryPath;

    private static readonly Lazy<LoadResult> libraryLoaded = new(() =>
    {
        return NativeLibraryLoader.LoadNativeLibrary(Gpt4AllModelFactory.libraryPath, Gpt4AllModelFactory.bypassLoading);
    }, true);

    public Gpt4AllModelFactory(string? libraryPath = default, bool bypassLoading = false)
    {
        Gpt4AllModelFactory.libraryPath = libraryPath;
        Gpt4AllModelFactory.bypassLoading = bypassLoading;

        if (!libraryLoaded.Value.IsSuccess)
        {
            throw new Exception($"Failed to load native gpt4all library. Error: {libraryLoaded.Value.ErrorMessage}");
        }
    }

    private static IGpt4AllModel CreateModel(string modelPath)
    {
        var modelType_ = ModelFileUtils.GetModelTypeFromModelFileHeader(modelPath);
        IntPtr error;
        var handle = NativeMethods.llmodel_model_create2(modelPath, "auto", out error);
        var loadedSuccesfully = NativeMethods.llmodel_loadModel(handle, modelPath);

        if (loadedSuccesfully == false)
        {
            throw new Exception($"Failed to load model: '{modelPath}'");
        }

        var underlyingModel = LLModel.Create(handle, modelType_);

        Debug.Assert(underlyingModel.IsLoaded());

        return new Gpt4All(underlyingModel);
    }

    public IGpt4AllModel LoadModel(string modelPath) => CreateModel(modelPath);
}
