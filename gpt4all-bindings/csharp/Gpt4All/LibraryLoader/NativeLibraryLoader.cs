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

    internal static LoadResult LoadNativeLibrary(string? path = default, bool bypassLoading = true)
    {
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

        // If the user hasn't set the path, we'll try to find it ourselves.
        if (string.IsNullOrEmpty(path))
        {
            var libraryName = "libllmodel";
            var assemblySearchPath = new[]
            {
                AppDomain.CurrentDomain.RelativeSearchPath,
                Path.GetDirectoryName(typeof(NativeLibraryLoader).Assembly.Location),
                Path.GetDirectoryName(Environment.GetCommandLineArgs()[0])
            }.FirstOrDefault(it => !string.IsNullOrEmpty(it));
            // Search for the library dll within the assembly search path. If it doesn't exist, for whatever reason, use the default path.
            path = Directory.EnumerateFiles(assemblySearchPath ?? string.Empty, $"{libraryName}.{extension}", SearchOption.AllDirectories).FirstOrDefault() ?? Path.Combine("runtimes", $"{platform}-{architecture}", $"{libraryName}.{extension}");
        }

        if (defaultLibraryLoader != null)
        {
            return defaultLibraryLoader.OpenLibrary(path);
        }

        if (!File.Exists(path))
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
        return libraryLoader.OpenLibrary(path);
    }
}
