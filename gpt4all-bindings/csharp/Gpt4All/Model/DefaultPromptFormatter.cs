namespace Gpt4All;

public class DefaultPromptFormatter : IPromptFormatter
{
    public string FormatPrompt(string prompt)
    {
        return $"""
        ### Instruction: 
        The prompt below is a question to answer, a task to complete, or a conversation
        to respond to; decide which and write an appropriate response.
        ### Prompt:
        {prompt}
        ### Response:
        """;
    }
}
