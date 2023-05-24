namespace Gpt4All;

public interface IGpt4AllModel : ITextPrediction, IDisposable
{
    /// <summary>
    /// The prompt formatter used to format the prompt before
    /// feeding it to the model, if null no transformation is applied
    /// </summary>
    IPromptFormatter? PromptFormatter { get; set; }
}
