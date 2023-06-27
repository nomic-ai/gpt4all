namespace Gpt4All;

public class ModelCreationException : Exception
{
    internal ModelCreationException(string modelFile, string? errorMessage, int errorCode) :
        base($"Failed to create model: '{modelFile}', reason='{errorMessage}' (0x{errorCode:X8})")
    {
    }

    internal ModelCreationException(string modelFile, string? errorMessage, int errorCode, Exception? innerException) :
        base($"Failed to create model: '{modelFile}', reason='{errorMessage}' (0x{errorCode:X8})", innerException)
    {
    }

    private ModelCreationException() : base()
    {
    }

    private ModelCreationException(string? message) : base(message)
    {
    }

    private ModelCreationException(string? message, Exception? innerException) : base(message, innerException)
    {
    }
}
