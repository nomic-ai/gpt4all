namespace Gpt4All.LibraryLoader;

public class LoadResult
{
    private LoadResult(bool isSuccess, string? errorMessage)
    {
        IsSuccess = isSuccess;
        ErrorMessage = errorMessage;
    }

    public static LoadResult Success { get; } = new(true, null);

    public static LoadResult Failure(string errorMessage)
    {
        return new(false, errorMessage);
    }

    public bool IsSuccess { get; }
    public string? ErrorMessage { get; }
}
