using System.IO;
using Gpt4All.LibraryLoader;
using Xunit;

namespace Gpt4All.Tests;

public class NativeLibraryLoaderTests
{
    [Fact]
    public void NativeLibraryShouldLoad()
    {
        var result = NativeLibraryLoader.LoadNativeLibrary(bypassLoading: false);
        Assert.True(result.IsSuccess);
    }

    private const string LLModelLib = "libllmodel.{0}";

    [PlatformSpecificFact(Platforms.Windows)]
    public void NativeLibraryShouldLoad_Windows()
    {
        var libraryLoader = new WindowsLibraryLoader();

        var libraryPath = Path.Combine(
            Environment.CurrentDirectory,
            string.Format(LLModelLib, "dll"));

        var result = libraryLoader.OpenLibrary(libraryPath);
        Assert.True(result.IsSuccess);
    }

    [PlatformSpecificFact(Platforms.Linux)]
    public void NativeLibraryShouldLoad_Linux()
    {
        var libraryLoader = new LinuxLibraryLoader();

        var libraryPath = Path.Combine(
            Environment.CurrentDirectory,
            string.Format(LLModelLib, "so"));

        var result = libraryLoader.OpenLibrary(libraryPath);
        Assert.True(result.IsSuccess);
    }

    [PlatformSpecificFact(Platforms.MacOS)]
    public void NativeLibraryShouldLoad_MacOS()
    {
        var libraryLoader = new MacOsLibraryLoader();

        var libraryPath = Path.Combine(
            Environment.CurrentDirectory,
            string.Format(LLModelLib, "dylib"));

        var result = libraryLoader.OpenLibrary(libraryPath);
        Assert.True(result.IsSuccess);
    }
}
