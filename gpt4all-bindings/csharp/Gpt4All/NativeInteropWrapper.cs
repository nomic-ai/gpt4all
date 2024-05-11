using System.Runtime.InteropServices;
using Gpt4All.Bindings;

namespace Gpt4All;
using System;

/// <summary>
/// Callback type for prompt processing.
/// </summary>
/// <param name="tokenId">The token id of the prompt.</param>
/// <returns>A bool indicating whether the model should keep processing.</returns>
internal delegate bool PromptCallback(int tokenId);

/// <summary>
/// Callback type for response.
/// </summary>
/// <param name="tokenId">The token id of the response.</param>
/// <param name="response">The response string. NOTE: a token_id of -1 indicates the string is an error string.</param>
/// <returns>A bool indicating whether the model should keep generating.</returns>
internal delegate bool ResponseCallback(int tokenId, string? response);

/// <summary>
/// Callback type for recalculation of context.
/// </summary>
/// <param name="isRecalculating">Whether the model is recalculating the context.</param>
/// <returns>A bool indicating whether the model should keep generating.</returns>
internal delegate bool RecalculateCallback(bool isRecalculating);

/// <summary>
/// Embedding cancellation callback for use with llmodel_embed.
/// </summary>
/// <param name="batchSizes">The number of tokens in each batch that will be embedded.</param>
/// <param name="numberOfBatches">The number of batches that will be embedded.</param>
/// <param name="backend">The backend that will be used for embedding. One of "cpu", "kompute", or "metal".</param>
/// <returns>True to cancel llmodel_embed, false to continue.</returns>
internal delegate bool EmbCancelCallback(uint[] batchSizes, uint numberOfBatches, string? backend);

/// <summary>
/// A wrapper around the auto-generated native interop methods.
/// </summary>
internal interface INativeInteropWrapper
{
    /// <summary>
    /// Create a llmodel instance.
    /// </summary>
    /// <param name="modelPath">Path to model</param>
    /// <param name="buildVariant">The implementation to use (auto, default, avxonly, ...)</param>
    /// <returns>A pointer to the llmodel_model instance</returns>
    /// <exception cref="Exception"></exception>
    public IntPtr ModelCreate(string modelPath, string buildVariant);

    /// <summary>
    /// Destroy a llmodel instance.
    /// </summary>
    /// <param name="model">A pointer to a llmodel_model instance</param>
    public void ModelDestroy(IntPtr model);

    /// <summary>
    /// Estimate RAM requirement for a model file.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <param name="modelPath">Path to model</param>
    /// <param name="maxContextSize">Maximum size of context window</param>
    /// <param name="numGpuLayers">Number of GPU layers to use (Vulkan)</param>
    /// <returns>RAM requirement size</returns>
    public long RequiredMem(IntPtr model, string modelPath, int maxContextSize, int numGpuLayers);

    /// <summary>
    /// Load a model from a file.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <param name="modelPath">Path to model</param>
    /// <param name="maxContextSize">Maximum size of context window</param>
    /// <param name="numGpuLayers">Number of GPU layers to use (Vulkan)</param>
    /// <returns>true if the model was loaded successfully, false otherwise.</returns>
    public bool LoadModel(IntPtr model, string modelPath, int maxContextSize, int numGpuLayers);

    /// <summary>
    /// Check if a model is loaded.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <returns>true if the model is loaded, false otherwise.</returns>
    public bool IsModelLoaded(IntPtr model);

    /// <summary>
    /// Get the size of the internal state of the model.
    /// NOTE: This state data is specific to the type of model you have created.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <returns>the size in bytes of the internal state of the model</returns>
    public ulong GetStateSize(IntPtr model);

    /// <summary>
    /// Saves the internal state of the model to a buffer.
    /// NOTE: This state data is specific to the type of model you have created.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <param name="size">Size of the state to save</param>
    /// <returns>The buffer containing the saved state</returns>
    public byte[] SaveStateData(IntPtr model, ulong size);

    /// <summary>
    /// Restores the internal state of the model using the given data.
    /// NOTE: This state data is specific to the type of model you have created.
    /// </summary>
    /// <param name="model">the llmodel_model instance</param>
    /// <param name="data">The buffer containing the saved state</param>
    /// <returns>the number of bytes read</returns>
    public void RestoreStateData(IntPtr model, byte[] data);

    /// <summary>
    /// Generate a response using the model.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <param name="prompt">Input prompt</param>
    /// <param name="promptTemplate">Input prompt template</param>
    /// <param name="promptCallback">A callback function for handling the processing of prompt</param>
    /// <param name="responseCallback">A callback function for handling the generated response</param>
    /// <param name="recalculateCallback">A callback function for handling recalculation requests</param>
    /// <param name="special">True if special tokens in the prompt should be processed, false otherwise</param>
    /// <param name="fakeReply">A string to insert into context as the model's reply, or NULL to generate one</param>
    /// <param name="context">The prompt context</param>
    public Models.PromptContext Prompt(IntPtr model, string prompt, string promptTemplate, PromptCallback promptCallback, ResponseCallback responseCallback, RecalculateCallback recalculateCallback, Models.PromptContext context, bool special, string? fakeReply);

    /// <summary>
    /// Generate an embedding using the model.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <param name="texts">Array of texts to generate an embedding for</param>
    /// <param name="prefix">The model-specific prefix representing the embedding task, without the trailing colon.
    /// NULL for no prefix</param>
    /// <param name="dimensionality">The embedding dimension, for use with Matryoshka-capable models. Set to -1 to for full-size</param>
    /// <param name="doMean">True to average multiple embeddings if the text is longer than the model can accept, False to
    /// truncate</param>
    /// <param name="atlas">Try to be fully compatible with the Atlas API. Currently, this means texts longer than 8192 tokens with
    /// long_text_mode="mean" will raise an error. Disabled by default</param>
    /// <param name="cancelCallback">Cancellation callback</param>
    /// <param name="tokenCount">The number of prompt tokens processed</param>
    /// <returns>The embedding</returns>
    /// <exception cref="Exception"></exception>
    public float[] Embed(
        IntPtr model,
        string[] texts,
        string? prefix,
        int dimensionality,
        bool doMean,
        bool atlas,
        EmbCancelCallback cancelCallback,
        out long tokenCount
    );

    /// <summary>
    /// Set the number of threads to be used by the model.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <param name="threadCount">The number of threads to be used</param>
    public void SetThreadCount(IntPtr model, int threadCount);

    /// <summary>
    /// Get the number of threads currently being used by the model.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <returns>The number of threads currently being used.</returns>
    public int ThreadCount(IntPtr model);

    /// <summary>
    /// Set llmodel implementation search path.
    /// Default is "."
    /// </summary>
    /// <param name="path">The path to the llmodel implementation shared objects. This can be a single path or
    /// a list of paths separated by ';' delimiter</param>
    public void SetImplementationSearchPath(string path);

    /// <summary>
    /// Get llmodel implementation search path.
    /// </summary>
    /// <returns>The current search path; lifetime ends on next set llmodel_set_implementation_search_path() call.</returns>
    public string? GetImplementationSearchPath();

    /// <summary>
    /// Get a list of available GPU devices given the memory required.
    /// </summary>
    /// <param name="memoryRequired">The minimum amount of VRAM, in bytes</param>
    /// <returns>A pointer to an array of llmodel_gpu_device's whose number is given by num_devices.</returns>
    public GPUDevice[] AvailableGpuDevices(long memoryRequired);

    /// <summary>
    /// Initializes a GPU device based on a specified string criterion.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <param name="memoryRequired">The amount of memory (in bytes) required by the application or task
    /// that will utilize the GPU device</param>
    /// <param name="device">A string specifying the desired criterion for GPU device selection. It can be:
    /// - "gpu": To initialize the best available GPU.
    /// - "amd", "nvidia", or "intel": To initialize the best available GPU from that vendor.
    /// - A specific GPU device name: To initialize a GPU with that exact name</param>
    /// <returns>True if the GPU device is successfully initialized based on the provided string
    /// criterion. Returns false if the desired GPU device could not be initialized.</returns>
    public bool GpuInitGpuDeviceByString(IntPtr model, long memoryRequired, string device);

    /// <summary>
    /// Initializes a GPU device by specifying a valid gpu device pointer.
    /// </summary>
    /// <param name="device">A gpu device pointer</param>
    /// <returns>True if the GPU device is successfully initialized, false otherwise.</returns>
    public bool GpuInitGpuDeviceByStruct(IntPtr model, GPUDevice device);

    /// <summary>
    /// Initializes a GPU device by its index.
    /// </summary>
    /// <param name="device">An integer representing the index of the GPU device to be initialized</param>
    /// <returns>True if the GPU device is successfully initialized, false otherwise.</returns>
    public bool GpuInitGpuDeviceByInt(IntPtr model, int device);

    /// <summary>
    /// Check if a GPU device is available.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <returns>True if a GPU device is available, false otherwise.</returns>
    public bool HasGpuDevice(IntPtr model);

    /// <summary>
    /// Get the name of the llama.cpp backend currently in use. One of "cpu", "kompute", or "metal".
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <returns>The name of the llama.cpp backend currently in use.</returns>
    public string? ModelBackendName(IntPtr model);

    /// <summary>
    /// Get the name of the GPU device currently in use, or NULL for backends other than Kompute.
    /// </summary>
    /// <param name="model">A pointer to the llmodel_model instance</param>
    /// <returns>The name of the GPU device currently in use, or NULL for backends other than Kompute.</returns>
    public string? ModelGpuDeviceName(IntPtr model);
}

internal class NativeInteropWrapper : INativeInteropWrapper
{
    public unsafe IntPtr ModelCreate(string modelPath, string buildVariant)
    {
        var error = IntPtr.Zero;
        var model = NativeMethods.llmodel_model_create2(StrToPtr(modelPath), StrToPtr(buildVariant), &error);

        if (error != IntPtr.Zero)
        {
            var errorMessage = PtrToStr(error);
            throw new Exception(errorMessage);
        }

        if (model == IntPtr.Zero)
        {
            throw new Exception("Error creating model");
        }

        return model;
    }

    public void ModelDestroy(IntPtr model)
    {
        NativeMethods.llmodel_model_destroy(model);
    }

    public long RequiredMem(IntPtr model, string modelPath, int nCtx, int ngl)
    {
        var size = (long)NativeMethods.llmodel_required_mem(model, StrToPtr(modelPath), nCtx, ngl);
        if (size <= 0)
        {
            throw new Exception("Error getting required memory");
        }

        return size;
    }

    public bool LoadModel(IntPtr model, string modelPath, int nCtx, int ngl)
    {
        return ByteToBool(NativeMethods.llmodel_loadModel(model, StrToPtr(modelPath), nCtx, ngl));
    }

    public bool IsModelLoaded(IntPtr model)
    {
        return ByteToBool(NativeMethods.llmodel_isModelLoaded(model));
    }

    public ulong GetStateSize(IntPtr model)
    {
        return NativeMethods.llmodel_get_state_size(model);
    }

    public unsafe byte[] SaveStateData(IntPtr model, ulong size)
    {
        var destinationArray = new byte[size];
        var destPtr = Marshal.UnsafeAddrOfPinnedArrayElement(destinationArray, 0);
        var written = NativeMethods.llmodel_save_state_data(model, (byte*)destPtr);
        if (written != size)
        {
            throw new Exception("Error saving state data");
        }

        return destinationArray;
    }

    public unsafe void RestoreStateData(IntPtr model, byte[] data)
    {
        var srcPtr = Marshal.UnsafeAddrOfPinnedArrayElement(data, 0);
        var written = NativeMethods.llmodel_restore_state_data(model, (byte*)srcPtr);
        if (written != (ulong)data.Length)
        {
            throw new Exception("Error restoring state data");
        }
    }

    public unsafe Models.PromptContext Prompt(nint model, string prompt, string promptTemplate,
        PromptCallback promptCallback, ResponseCallback responseCallback,
        RecalculateCallback recalculateCallback, Models.PromptContext context,
        bool special, string? fakeReply)
    {
        var nativeContext = ToContext(context);

        NativeMethods.llmodel_prompt(
            model,
            StrToPtr(prompt),
            StrToPtr(promptTemplate),
            (tokenId) => promptCallback(tokenId),
            (tokenId, response) =>
            {
                var responseStr = PtrToStr(response);
                return responseCallback(tokenId, responseStr);
            },
            (isRecalculating) => recalculateCallback(isRecalculating),
            &nativeContext,
            BoolToByte(special),
            StrToPtr(fakeReply)
        );

        return FromContext(nativeContext);
    }

    public unsafe float[] Embed(IntPtr model, string[] texts, string? prefix, int dimensionality, bool doMean, bool atlas, EmbCancelCallback cancelCallback, out long tokenCount)
    {
        string[] strings = texts.ToArray();
        IntPtr[] stringsPtrArr = StrArrayToPtrArray(texts);
        IntPtr* stringsPtr = (IntPtr*)Marshal.UnsafeAddrOfPinnedArrayElement(stringsPtrArr, 0);
        IntPtr prefixPtr = StrToPtr(prefix);
        byte b_doMean = BoolToByte(doMean);
        byte b_atlas = BoolToByte(atlas);
        nuint embeddingSize;
        nuint internalTokenCount;
        IntPtr error;

        var embedPtr = NativeMethods.llmodel_embed(model, stringsPtr, &embeddingSize, prefixPtr, dimensionality, &internalTokenCount, b_doMean, b_atlas, (z, a, c) => false, &error);

        if (error != IntPtr.Zero)
        {
            var errorMessage = PtrToStr(error);
            throw new Exception(errorMessage);
        }

        if (embedPtr == default)
        {
            tokenCount = 0;
            return Array.Empty<float>();
        }

        tokenCount = (long)internalTokenCount;
        var retArray = PtrToArrayOf<float>((IntPtr)embedPtr, (long)embeddingSize);

        NativeMethods.llmodel_free_embedding((float*)embedPtr);

        if (retArray is null)
        {
            throw new Exception("Error embedding text");
        }

        return retArray;
    }

    public void SetThreadCount(nint model, int threadCount)
    {
        NativeMethods.llmodel_setThreadCount(model, threadCount);
    }

    public int ThreadCount(nint model)
    {
        return NativeMethods.llmodel_threadCount(model);
    }

    public void SetImplementationSearchPath(string path)
    {
        NativeMethods.llmodel_set_implementation_search_path(StrToPtr(path));
    }

    public unsafe string? GetImplementationSearchPath()
    {
        var response = NativeMethods.llmodel_get_implementation_search_path();
        return PtrToStr(response);
    }

    public unsafe GPUDevice[] AvailableGpuDevices(long memoryRequired)
    {
        int numDevices;
        var response = (IntPtr)NativeMethods.llmodel_available_gpu_devices((nuint)memoryRequired, &numDevices);
        return PtrToArrayOf<GPUDevice>(response, numDevices);
    }

    public bool GpuInitGpuDeviceByString(IntPtr model, long memoryRequired, string device)
    {
        return ByteToBool(NativeMethods.llmodel_gpu_init_gpu_device_by_string(model, (nuint)memoryRequired, StrToPtr(device)));
    }

    public unsafe bool GpuInitGpuDeviceByStruct(IntPtr model, GPUDevice device)
    {
        return ByteToBool(NativeMethods.llmodel_gpu_init_gpu_device_by_struct(model, &device));
    }

    public bool GpuInitGpuDeviceByInt(IntPtr model, int device)
    {
        return ByteToBool(NativeMethods.llmodel_gpu_init_gpu_device_by_int(model, device));
    }

    public bool HasGpuDevice(IntPtr model)
    {
        return ByteToBool(NativeMethods.llmodel_has_gpu_device(model));
    }

    public string? ModelBackendName(IntPtr model)
    {
        var response = NativeMethods.llmodel_model_backend_name(model);
        return PtrToStr(response);
    }

    public string? ModelGpuDeviceName(IntPtr model)
    {
        var response = NativeMethods.llmodel_model_gpu_device_name(model);
        return PtrToStr(response);
    }

    private static T[] PtrToArrayOf<T>(IntPtr unmanagedArray, long length)
    {
        var size = Marshal.SizeOf(typeof(T));
        var mangagedArray = new T[length];

        for (var i = 0; i < length; i++)
        {
            var ins = new IntPtr(unmanagedArray.ToInt64() + i * size);
            var marshalled = Marshal.PtrToStructure<T>(ins);
            if (marshalled is null)
            {
                throw new Exception("Error marshalling unmanaged array to struct");
            }

            mangagedArray[i] = marshalled;
        }

        return mangagedArray;
    }

    private static IntPtr[] StrArrayToPtrArray(string[] array)
    {
        var mangagedArray = new IntPtr[array.Length];
        for (var i = 0; i < array.Length; i++)
        {
            mangagedArray[i] = StrToPtr(array[i]);
        }

        return mangagedArray;
    }

    private static unsafe Bindings.PromptContext ToContext(Models.PromptContext context)
    {
        var nativeContext = new Bindings.PromptContext
        {
            context_erase = context.ContextErase,
            min_p = context.MinP,
            n_batch = context.NBatch,
            n_ctx = context.NCtx,
            n_past = context.NPast,
            n_predict = context.NPredict,
            repeat_last_n = context.RepeatLastN,
            repeat_penalty = context.RepeatPenalty,
            temp = context.Temp,
            top_k = context.TopK,
            top_p = context.TopP,
        };

        if (context?.Logits is not null)
        {
            var logits = context.Logits.ToArray();
            nativeContext.logits_size = (nuint)logits.Length;
            nativeContext.logits = (float*)Marshal.UnsafeAddrOfPinnedArrayElement(logits, 0);
        }

        if (context?.Tokens is not null)
        {
            var tokens = context.Tokens.ToArray();
            nativeContext.tokens_size = (nuint)tokens.Length;
            nativeContext.tokens = (int*)Marshal.UnsafeAddrOfPinnedArrayElement(tokens, 0);
        }

        return nativeContext;
    }

    private static unsafe Models.PromptContext FromContext(Bindings.PromptContext nativeContext)
    {
        // Map from context
        var context = new Models.PromptContext
        {
            ContextErase = nativeContext.context_erase,
            MinP = nativeContext.min_p,
            NBatch = nativeContext.n_batch,
            NCtx = nativeContext.n_ctx,
            NPast = nativeContext.n_past,
            NPredict = nativeContext.n_predict,
            RepeatLastN = nativeContext.repeat_last_n,
            RepeatPenalty = nativeContext.repeat_penalty,
            Temp = nativeContext.temp,
            TopK = nativeContext.top_k,
            TopP = nativeContext.top_p,
        };

        var logitsPtr = (IntPtr)nativeContext.logits;
        if (logitsPtr != IntPtr.Zero && nativeContext.logits_size > 0)
        {
            context.Logits = PtrToArrayOf<float>(logitsPtr, (long)nativeContext.logits_size);
        }

        var tokensPtr = (IntPtr)nativeContext.tokens;
        if (tokensPtr != IntPtr.Zero && nativeContext.tokens_size > 0)
        {
            context.Tokens = PtrToArrayOf<int>(tokensPtr, (long)nativeContext.tokens_size);
        }

        return context;
    }

    private static byte BoolToByte(bool b) => (byte)(b ? 1 : 0);
    private static bool ByteToBool(byte b) => b == 1;
    private static IntPtr StrToPtr(string? text) => Marshal.StringToHGlobalAnsi(text);
    private static string? PtrToStr(IntPtr ptr) => Marshal.PtrToStringAnsi(ptr);
}
