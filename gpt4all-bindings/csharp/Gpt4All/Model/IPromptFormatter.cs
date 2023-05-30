namespace Gpt4All;

/// <summary>
/// Formats a prompt
/// </summary>
public interface IPromptFormatter
{
    /// <summary>
    /// Format the provided prompt
    /// </summary>
    /// <param name="prompt">the input prompt</param>
    /// <returns>The formatted prompt</returns>
    string FormatPrompt(string prompt);
}
