namespace Gpt4All.LibraryLoader;

public interface ILibraryLoader
{
    LoadResult OpenLibrary(string? fileName);
}
