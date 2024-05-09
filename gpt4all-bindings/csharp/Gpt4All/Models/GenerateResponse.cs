namespace Gpt4All.Models;

public class GenerateResponse
{
    public GenerateResponse(string response, int totalTokens, PromptContext promptContext)
    {
        Response = response;
        TotalTokens = totalTokens;
        PromptContext = promptContext;
    }

    public string Response { get; set; }

    public int TotalTokens { get; set; }

    public PromptContext PromptContext { get; }
}
