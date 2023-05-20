namespace Gpt4All;

public interface IGpt4AllModelFactory
{
    IGpt4AllModel LoadGptjModel(string modelPath);

    IGpt4AllModel LoadLlamaModel(string modelPath);

    IGpt4AllModel LoadModel(string modelPath);

    IGpt4AllModel LoadMptModel(string modelPath);
}
