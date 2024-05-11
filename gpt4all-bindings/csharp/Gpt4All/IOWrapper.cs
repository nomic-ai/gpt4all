namespace Gpt4All;

internal interface IIOWrapper
{
    bool FileExists(string path);
}

internal class IOWrapper : IIOWrapper
{
    public bool FileExists(string path)
    {
        return File.Exists(path);
    }
}
