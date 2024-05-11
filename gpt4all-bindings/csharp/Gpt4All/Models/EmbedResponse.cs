namespace Gpt4All.Models;

public class EmbedResponse
{
    public IEnumerable<float> Embeddings { get; set; } = new List<float>();

    public long TokenCount { get; set; }
}
