namespace Gpt4All.Bindings;

internal partial struct GPUDevice
{
    public int index;

    public int type;

    [NativeTypeName("size_t")]
    public UIntPtr heapSize;

    [NativeTypeName("const char *")]
    public IntPtr name;

    [NativeTypeName("const char *")]
    public IntPtr vendor;
}
