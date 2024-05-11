namespace Gpt4All.Models;

public class GenerateResponse
{
    public string? Response { get; set; }

    public int TotalTokens { get; set; }

    public PromptContext? PromptContext { get; set; }
}
