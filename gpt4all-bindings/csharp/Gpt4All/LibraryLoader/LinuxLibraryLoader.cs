using System.Runtime.InteropServices;

namespace Gpt4All.LibraryLoader;

internal class LinuxLibraryLoader : ILibraryLoader
{
#pragma warning disable CA2101
    [DllImport("libdl.so", ExactSpelling = true, CharSet = CharSet.Auto, EntryPoint = "dlopen")]
#pragma warning restore CA2101
    public static extern IntPtr NativeOpenLibraryLibdl(string? filename, int flags);

#pragma warning disable CA2101
    [DllImport("libdl.so.2", ExactSpelling = true, CharSet = CharSet.Auto, EntryPoint = "dlopen")]
#pragma warning restore CA2101
    public static extern IntPtr NativeOpenLibraryLibdl2(string? filename, int flags);

    [DllImport("libdl.so", ExactSpelling = true, CharSet = CharSet.Auto, EntryPoint = "dlerror")]
    public static extern IntPtr GetLoadError();

    [DllImport("libdl.so.2", ExactSpelling = true, CharSet = CharSet.Auto, EntryPoint = "dlerror")]
    public static extern IntPtr GetLoadError2();

    public LoadResult OpenLibrary(string? fileName)
    {
        IntPtr loadedLib;
        try
        {
            // open with rtls lazy flag
            loadedLib = NativeOpenLibraryLibdl2(fileName, 0x00001);
        }
        catch (DllNotFoundException)
        {
            loadedLib = NativeOpenLibraryLibdl(fileName, 0x00001);
        }

        if (loadedLib == IntPtr.Zero)
        {
            string errorMessage;
            try
            {
                errorMessage = Marshal.PtrToStringAnsi(GetLoadError2()) ?? "Unknown error";
            }
            catch (DllNotFoundException)
            {
                errorMessage = Marshal.PtrToStringAnsi(GetLoadError()) ?? "Unknown error";
            }

            return LoadResult.Failure(errorMessage);
        }

        return LoadResult.Success;
    }
}
