using System.Runtime.InteropServices;

namespace Gpt4All.LibraryLoader;

internal class MacOsLibraryLoader : ILibraryLoader
{
#pragma warning disable CA2101
    [DllImport("libdl.dylib", ExactSpelling = true, CharSet = CharSet.Auto, EntryPoint = "dlopen")]
#pragma warning restore CA2101
    public static extern IntPtr NativeOpenLibraryLibdl(string? filename, int flags);

    [DllImport("libdl.dylib", ExactSpelling = true, CharSet = CharSet.Auto, EntryPoint = "dlerror")]
    public static extern IntPtr GetLoadError();

    public LoadResult OpenLibrary(string? fileName)
    {
        var loadedLib = NativeOpenLibraryLibdl(fileName, 0x00001);

        if (loadedLib == IntPtr.Zero)
        {
            var errorMessage = Marshal.PtrToStringAnsi(GetLoadError()) ?? "Unknown error";

            return LoadResult.Failure(errorMessage);
        }

        return LoadResult.Success;
    }
}
