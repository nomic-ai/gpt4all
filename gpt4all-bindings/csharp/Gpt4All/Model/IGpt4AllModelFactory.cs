namespace Gpt4All;

public interface IGpt4AllModelFactory
{
    IGpt4AllModel LoadModel(string modelPath);
}
