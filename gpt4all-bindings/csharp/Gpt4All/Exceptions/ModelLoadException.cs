namespace Gpt4All;

public class ModelLoadException : Exception
{
    public ModelLoadException(string modelFile) :
        base($"Failed to load model: '{modelFile}'")
    {
    }

    private ModelLoadException() : base()
    {
    }

    public ModelLoadException(string? modelFile, Exception? innerException) :
        base($"Failed to load model: '{modelFile}'", innerException)
    {
    }
}
