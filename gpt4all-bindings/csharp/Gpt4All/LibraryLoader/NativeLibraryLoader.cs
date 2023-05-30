#if !IOS && !MACCATALYST && !TVOS && !ANDROID
using System.Runtime.InteropServices;
#endif

namespace Gpt4All.LibraryLoader;

public static class NativeLibraryLoader
{
    private static ILibraryLoader? defaultLibraryLoader;

    /// <summary>
    /// Sets the library loader used to load the native libraries. Overwrite this only if you want some custom loading.
    /// </summary>
    /// <param name="libraryLoader">The library loader to be used.</param>
    public static void SetLibraryLoader(ILibraryLoader libraryLoader)
    {
        defaultLibraryLoader = libraryLoader;
    }

    public static LoadResult LoadNativeLibrary(string? path = default, bool bypassLoading = false)
    {
#if IOS || MACCATALYST || TVOS || ANDROID
        // If we're not bypass loading, and the path was set, and loader was set, allow it to go through.
        if (!bypassLoading && defaultLibraryLoader != null)
        {
            return defaultLibraryLoader.OpenLibrary(path);
        }

        return LoadResult.Success;
#else
        // If the user has handled loading the library themselves, we don't need to do anything.
        if (bypassLoading)
        {
            return LoadResult.Success;
        }

        var architecture = RuntimeInformation.OSArchitecture switch
        {
            Architecture.X64 => "x64",
            Architecture.X86 => "x86",
            Architecture.Arm => "arm",
            Architecture.Arm64 => "arm64",
            _ => throw new PlatformNotSupportedException(
                $"Unsupported OS platform, architecture: {RuntimeInformation.OSArchitecture}")
        };

        var (platform, extension) = Environment.OSVersion.Platform switch
        {
            _ when RuntimeInformation.IsOSPlatform(OSPlatform.Windows) => ("win", "dll"),
            _ when RuntimeInformation.IsOSPlatform(OSPlatform.Linux) => ("linux", "so"),
            _ when RuntimeInformation.IsOSPlatform(OSPlatform.OSX) => ("osx", "dylib"),
            _ => throw new PlatformNotSupportedException(
                $"Unsupported OS platform, architecture: {RuntimeInformation.OSArchitecture}")
        };

        var libllmodel = string.Empty;

        if (string.IsNullOrEmpty(path))
        {
            var assemblySearchPath = new[]
            {
                AppDomain.CurrentDomain.RelativeSearchPath,
                Path.GetDirectoryName(typeof(NativeLibraryLoader).Assembly.Location),
                Path.GetDirectoryName(Environment.GetCommandLineArgs()[0])
            }.FirstOrDefault(it => !string.IsNullOrEmpty(it));

            libllmodel = string.IsNullOrEmpty(assemblySearchPath)
                ? Path.Combine("runtimes", $"{platform}-{architecture}", $"libllmodel.{extension}")
                : Path.Combine(assemblySearchPath, "runtimes", $"{platform}-{architecture}", $"libllmodel.{extension}");
        }

        if (defaultLibraryLoader != null)
        {
            return defaultLibraryLoader.OpenLibrary(path);
        }

        if (!File.Exists(libllmodel))
        {
            throw new FileNotFoundException($"Native Library not found in path {path}. " +
                                            $"Verify you have have included the native Gpt4All library in your application.");
        }

        ILibraryLoader libraryLoader = platform switch
        {
            "win" => new WindowsLibraryLoader(),
            "osx" => new MacOsLibraryLoader(),
            "linux" => new LinuxLibraryLoader(),
            _ => throw new PlatformNotSupportedException($"Currently {platform} platform is not supported")
        };

        var result = libraryLoader.OpenLibrary(libllmodel);

        return result;
#endif
    }
}
