namespace Gpt4All;

public static class ModelFileUtils
{
    private const uint GPTJ_MAGIC = 0x67676d6c;
    private const uint LLAMA_MAGIC = 0x67676a74;
    private const uint MPT_MAGIC = 0x67676d6d;

    public static ModelType GetModelTypeFromModelFileHeader(string modelPath)
    {
        using var fileStream = new FileStream(modelPath, FileMode.Open);
        using var binReader = new BinaryReader(fileStream);

        var magic = binReader.ReadUInt32();

        return magic switch
        {
            GPTJ_MAGIC => ModelType.GPTJ,
            LLAMA_MAGIC => ModelType.LLAMA,
            MPT_MAGIC => ModelType.MPT,
            _ => throw new ArgumentOutOfRangeException($"Invalid model file. magic=0x{magic:X8}"),
        };
    }
}
