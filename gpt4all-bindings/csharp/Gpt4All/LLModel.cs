using System.Runtime.InteropServices;
using System.Text;
using Gpt4All.Bindings;
using Gpt4All.Models;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Logging.Abstractions;

namespace Gpt4All;

public class LLModel : IDisposable
{
    private readonly string _model;
    private readonly ILogger _logger;

    private IntPtr _handle;
    private bool _loaded;

    public LLModel(string model, ILogger? logger = null)
    {
        _model = model;
        _logger = logger ?? NullLogger.Instance;
    }

    public void Dispose()
    {
        GC.SuppressFinalize(this);
        DestroyModel();
    }

    public unsafe IEnumerable<float> Embed(IEnumerable<string> stringToEmbed, int dimensionality = 768, string? prefix = null, bool doMean = false, bool atlas = false)
    {
        LoadModel();

        var strings = stringToEmbed.ToArray();
        IntPtr[] stringsPtrArr;
        Marshal2UnmananagedArray(strings, out stringsPtrArr);
        var stringsPtr = (nint*)Marshal.UnsafeAddrOfPinnedArrayElement(stringsPtrArr, 0);
        var prefixPtr = prefix is not null ? StrToPtr(prefix) : IntPtr.Zero;
        byte b_doMean = doMean ? (byte)1 : (byte)0;
        byte b_atlas = atlas ? (byte)1 : (byte)0;

        nuint embeddingSize;
        nuint tokenCount;
        IntPtr error;
        var embeds = NativeMethods.llmodel_embed(_handle, stringsPtr, &embeddingSize, prefixPtr, dimensionality, &tokenCount, b_doMean, b_atlas, IntPtr.Zero, &error);

        float[] embeddings;
        MarshalUnmananagedArray2Struct((nint)embeds, (int)embeddingSize, out embeddings);
        return embeddings;
    }

    public unsafe GenerateResponse Generate(
        string prompt,
        string template,
        Models.PromptContext? context = null,
        CancellationToken cancelToken = default)
    {
        LoadModel();

        // Map from context
        context ??= new Models.PromptContext();
        var ctx = new Bindings.PromptContext
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
            float[] logits = context.Logits.ToArray();
            ctx.logits_size = (nuint)logits.Length;
            ctx.logits = (float*)Marshal.UnsafeAddrOfPinnedArrayElement(logits, 0);
        }

        if (context?.Tokens is not null)
        {
            int[] tokens = context.Tokens.ToArray();
            ctx.tokens_size = (nuint)tokens.Length;
            ctx.tokens = (int*)Marshal.UnsafeAddrOfPinnedArrayElement(tokens, 0);
        }

        var totalTokens = 0;
        var builder = new StringBuilder();

        NativeMethods.llmodel_prompt(
            _handle,
            StrToPtr(prompt),
            StrToPtr(template),
            (tokenId) =>
            {
                totalTokens++;
                return !cancelToken.IsCancellationRequested;
            },
            (tokenId, response) =>
            {
                _logger.LogInformation("Token ID: {TokenId}", tokenId);
                totalTokens++;
                builder.Append(PtrToStr(response));
                return !cancelToken.IsCancellationRequested;
            },
            (isRecalculating) =>
            {
                return !cancelToken.IsCancellationRequested;
            },
            &ctx,
            0,
            IntPtr.Zero
        );

        // Map to context
        context = new Models.PromptContext
        {
            ContextErase = ctx.context_erase,
            MinP = ctx.min_p,
            NBatch = ctx.n_batch,
            NCtx = ctx.n_ctx,
            NPast = ctx.n_past,
            NPredict = ctx.n_predict,
            RepeatLastN = ctx.repeat_last_n,
            RepeatPenalty = ctx.repeat_penalty,
            Temp = ctx.temp,
            TopK = ctx.top_k,
            TopP = ctx.top_p,
        };

        if (ctx.logits != default)
        {
            float[] logits;
            MarshalUnmananagedArray2Struct((nint)ctx.logits, (int)ctx.logits_size, out logits);
            context.Logits = logits;
        }

        if (ctx.tokens != default)
        {
            int[] tokens;
            MarshalUnmananagedArray2Struct((nint)ctx.tokens, (int)ctx.tokens_size, out tokens);
            context.Tokens = tokens;
        }

        return new GenerateResponse(builder.ToString(), totalTokens, context);
    }

    private unsafe void DestroyModel()
    {
        if (_loaded && _handle != IntPtr.Zero)
        {
            NativeMethods.llmodel_model_destroy(_handle);
            _handle = IntPtr.Zero;
            _logger.LogInformation("Model destroyed");
        }
    }

    private unsafe void LoadModel(int nCtx = 2048, int ngl = 100)
    {
        if (_loaded)
        {
            return;
        }

        var handle = CreateModel();
        var loaded = NativeMethods.llmodel_loadModel(handle, StrToPtr(_model), nCtx, ngl);
        if (!Bool(loaded))
        {
            throw new Exception("Error loading model");
        }

        _loaded = true;
    }

    private unsafe IntPtr CreateModel(string buildVariant = "auto")
    {
        // TODO: Ensure file exists

        var pathToExecutable = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location)!;
        NativeMethods.llmodel_set_implementation_search_path(StrToPtr(pathToExecutable));

        var error_ptr = IntPtr.Zero;
        _handle = NativeMethods.llmodel_model_create2(StrToPtr(_model), StrToPtr(buildVariant), &error_ptr);
        if (error_ptr != IntPtr.Zero)
        {
            var error = Marshal.PtrToStringAnsi(error_ptr) ?? "Error creating model";
            throw new Exception(error);
        }

        if (_handle == IntPtr.Zero)
        {
            throw new Exception("Error creating model");
        }

        _logger.LogInformation("Model created successfully");
        return _handle;
    }

    private static void MarshalUnmananagedArray2Struct<T>(IntPtr unmanagedArray, int length, out T[] mangagedArray)
    {
        var size = Marshal.SizeOf(typeof(T));
        mangagedArray = new T[length];

        for (int i = 0; i < length; i++)
        {
            IntPtr ins = new IntPtr(unmanagedArray.ToInt64() + i * size);
            var marshalled = Marshal.PtrToStructure<T>(ins);
            if (marshalled is null)
            {
                throw new Exception("Error marshalling unmanaged array to struct");
            }
            mangagedArray[i] = marshalled;
        }
    }

    private static void Marshal2UnmananagedArray(string[] array, out IntPtr[] mangagedArray)
    {
        mangagedArray = new IntPtr[array.Length];
        for (int i = 0; i < array.Length; i++)
        {
            var marshalled = StrToPtr(array[i]);
            mangagedArray[i] = marshalled;
        }
    }

    private static bool Bool(byte b) => b == 1;
    private static IntPtr StrToPtr(string text) => Marshal.StringToHGlobalAnsi(text);
    private static string? PtrToStr(IntPtr ptr) => Marshal.PtrToStringAnsi(ptr);
}
