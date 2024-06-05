/// <reference types="node" />
declare module "gpt4all";

interface LLModelOptions {
    /**
     * Model architecture. This argument currently does not have any functionality and is just used as descriptive identifier for user.
     */
    modelType?: string;
    /**
     * Absolute path to the model file.
     */
    modelFile: string;
    /**
     * Path to the llmodel implementation shared objects. This can be a single path or a list of paths separated by ';' delimiter.
     */
    librariesPath?: string;
    /**
     * A string representing the implementation to use. One of 'auto', 'cpu', 'metal', 'kompute', or 'cuda'.
     */
    backend: string;
    /**
     * The maximum window size of this model.
     */
    nCtx: number;
    /**
     * Number of GPU layers to use (Vulkan)
     */
    nGpuLayers: number;
}

interface ModelConfig {
    systemPrompt: string;
    promptTemplate: string;
    path: string;
    url?: string;
}

/**
 * Options for the chat session.
 */
interface ChatSessionOptions extends Partial<LLModelPromptContext> {
    /**
     * System prompt to ingest on initialization.
     */
    systemPrompt?: string;

    /**
     * Messages to ingest on initialization.
     */
    messages?: ChatMessage[];
}

/**
 * ChatSession utilizes an InferenceModel for efficient processing of chat conversations.
 */
declare class ChatSession implements CompletionProvider {
    /**
     * Constructs a new ChatSession using the provided InferenceModel and options.
     * Does not set the chat session as the active chat session until initialize is called.
     * @param {InferenceModel} model An InferenceModel instance.
     * @param {ChatSessionOptions} [options] Options for the chat session including default completion options.
     */
    constructor(model: InferenceModel, options?: ChatSessionOptions);
    /**
     * The underlying InferenceModel used for generating completions.
     */
    model: InferenceModel;
    /**
     * The name of the model.
     */
    modelName: string;
    /**
     * The messages that have been exchanged in this chat session.
     */
    messages: ChatMessage[];
    /**
     * The system prompt that has been ingested at the beginning of the chat session.
     */
    systemPrompt: string;
    /**
     * The current prompt context of the chat session.
     */
    promptContext: LLModelPromptContext;

    /**
     * Ingests system prompt and initial messages.
     * Sets this chat session as the active chat session of the model.
     * @param {CompletionOptions} [options] Set completion options for initialization.
     * @returns {Promise<number>} The number of tokens ingested during initialization. systemPrompt + messages.
     */
    initialize(completionOpts?: CompletionOptions): Promise<number>;

    /**
     * Prompts the model in chat-session context.
     * @param {CompletionInput} input Input string or message array.
     * @param {CompletionOptions} [options] Set completion options for this generation.
     * @returns {Promise<InferenceResult>} The inference result.
     * @throws {Error} If the chat session is not the active chat session of the model.
     * @throws {Error} If nPast is set to a value higher than what has been ingested in the session.
     */
    generate(
        input: CompletionInput,
        options?: CompletionOptions
    ): Promise<InferenceResult>;
}

/**
 * Shape of InferenceModel generations.
 */
interface InferenceResult extends LLModelInferenceResult {
    tokensIngested: number;
    tokensGenerated: number;
}

/**
 * InferenceModel represents an LLM which can make next-token predictions.
 */
declare class InferenceModel implements CompletionProvider {
    constructor(llm: LLModel, config: ModelConfig);
    /** The native LLModel */
    llm: LLModel;
    /** The configuration the instance was constructed with. */
    config: ModelConfig;
    /** The active chat session of the model. */
    activeChatSession?: ChatSession;
    /** The name of the model. */
    modelName: string;

    /**
     * Create a chat session with the model and set it as the active chat session of this model.
     * A model instance can only have one active chat session at a time.
     * @param {ChatSessionOptions} options The options for the chat session.
     * @returns {Promise<ChatSession>} The chat session.
     */
    createChatSession(options?: ChatSessionOptions): Promise<ChatSession>;

    /**
     * Prompts the model with a given input and optional parameters.
     * @param {CompletionInput} input The prompt input.
     * @param {CompletionOptions} options Prompt context and other options.
     * @returns {Promise<InferenceResult>} The model's response to the prompt.
     * @throws {Error} If nPast is set to a value smaller than 0.
     * @throws {Error} If a messages array without a tailing user message is provided.
     */
    generate(
        prompt: string,
        options?: CompletionOptions
    ): Promise<InferenceResult>;

    /**
     * delete and cleanup the native model
     */
    dispose(): void;
}

/**
 * Options for generating one or more embeddings.
 */
interface EmbedddingOptions {
    /**
     * The model-specific prefix representing the embedding task, without the trailing colon. For Nomic Embed
     * this can be `search_query`, `search_document`, `classification`, or `clustering`.
     */
    prefix?: string;
    /**
     *The embedding dimension, for use with Matryoshka-capable models. Defaults to full-size.
     * @default determines on the model being used.
     */
    dimensionality?: number;
    /**
     * How to handle texts longer than the model can accept. One of `mean` or `truncate`.
     * @default "mean"
     */
    longTextMode?: "mean" | "truncate";
    /**
     * Try to be fully compatible with the Atlas API. Currently, this means texts longer than 8192 tokens
     * with long_text_mode="mean" will raise an error. Disabled by default.
     * @default false
     */
    atlas?: boolean;
}

/**
 * The nodejs moral equivalent to python binding's Embed4All().embed()
 * meow
 * @param {EmbeddingModel} model The embedding model instance.
 * @param {string} text Text to embed.
 * @param {EmbeddingOptions} options Optional parameters for the embedding.
 * @returns {EmbeddingResult} The embedding result.
 * @throws {Error} If dimensionality is set to a value smaller than 1.
 */
declare function createEmbedding(
    model: EmbeddingModel,
    text: string,
    options?: EmbedddingOptions
): EmbeddingResult<Float32Array>;

/**
 * Overload that takes multiple strings to embed.
 * @param {EmbeddingModel} model The embedding model instance.
 * @param {string[]} texts Texts to embed.
 * @param {EmbeddingOptions} options Optional parameters for the embedding.
 * @returns {EmbeddingResult<Float32Array[]>} The embedding result.
 * @throws {Error} If dimensionality is set to a value smaller than 1.
 */
declare function createEmbedding(
    model: EmbeddingModel,
    text: string[],
    options?: EmbedddingOptions
): EmbeddingResult<Float32Array[]>;

/**
 * The resulting embedding.
 */
interface EmbeddingResult<T> {
    /**
     * Encoded token count. Includes overlap but specifically excludes tokens used for the prefix/task_type, BOS/CLS token, and EOS/SEP token
     **/
    n_prompt_tokens: number;

    embeddings: T;
}
/**
 * EmbeddingModel represents an LLM which can create embeddings, which are float arrays
 */
declare class EmbeddingModel {
    constructor(llm: LLModel, config: ModelConfig);
    /** The native LLModel */
    llm: LLModel;
    /** The configuration the instance was constructed with. */
    config: ModelConfig;

    /**
     * Create an embedding from a given input string. See EmbeddingOptions.
     * @param {string} text
     * @param {string} prefix
     * @param {number} dimensionality
     * @param {boolean} doMean
     * @param {boolean} atlas
     * @returns {EmbeddingResult<Float32Array>} The embedding result.
     */
    embed(
        text: string,
        prefix: string,
        dimensionality: number,
        doMean: boolean,
        atlas: boolean
    ): EmbeddingResult<Float32Array>;
    /**
     * Create an embedding from a given input text array. See EmbeddingOptions.
     * @param {string[]} text
     * @param {string} prefix
     * @param {number} dimensionality
     * @param {boolean} doMean
     * @param {boolean} atlas
     * @returns {EmbeddingResult<Float32Array[]>} The embedding result.
     */
    embed(
        text: string[],
        prefix: string,
        dimensionality: number,
        doMean: boolean,
        atlas: boolean
    ): EmbeddingResult<Float32Array[]>;

    /**
     * delete and cleanup the native model
     */
    dispose(): void;
}

/**
 * Shape of LLModel's inference result.
 */
interface LLModelInferenceResult {
    text: string;
    nPast: number;
}

interface LLModelInferenceOptions extends Partial<LLModelPromptContext> {
    /** Callback for response tokens, called for each generated token.
     * @param {number} tokenId The token id.
     * @param {Uint8Array} bytes The token bytes.
     * @returns {boolean | undefined} Whether to continue generating tokens.
     * */
    onResponseToken?: (tokenId: number, bytes: Uint8Array) => boolean | void;
    /** Callback for prompt tokens, called for each input token in the prompt.
     * @param {number} tokenId The token id.
     * @returns {boolean | undefined} Whether to continue ingesting the prompt.
     * */
    onPromptToken?: (tokenId: number) => boolean | void;
}

/**
 * LLModel class representing a language model.
 * This is a base class that provides common functionality for different types of language models.
 */
declare class LLModel {
    /**
     * Initialize a new LLModel.
     * @param {LLModelOptions} options LLModel options.
     * @throws {Error} If the model can't be loaded or necessary runtimes are not found.
     */
    constructor(options: LLModelOptions);
    /**
     * Loads the LLModel.
     * @return {boolean} true if the model was loaded successfully, false otherwise.
     */
    load(): boolean;
    
    /**
     * Initiate a GPU by a string identifier. See LoadModelOptions.device for more information
     * @param {string} device  'amd' | 'nvidia' | 'intel' | 'gpu' | gpu name.
     * @return {boolean} true if the GPU was initialized successfully, false otherwise.
     */
    initGpu(device: string): boolean;

    /** undefined or user supplied */
    getType(): string | undefined;

    /** The name of the model. */
    getName(): string;

    /**
     * Get the size of the internal state of the model.
     * NOTE: This state data is specific to the type of model you have created.
     * @return the size in bytes of the internal state of the model
     */
    getStateSize(): number;

    /**
     * Get the number of threads used for model inference.
     * The default is the number of physical cores your computer has.
     * @returns The number of threads used for model inference.
     */
    getThreadCount(): number;

    /**
     * Set the number of threads used for model inference.
     * @param newNumber The new number of threads.
     */
    setThreadCount(newNumber: number): void;

    /**
     * Prompt the model directly with a given input string and optional parameters.
     * Use the higher level createCompletion methods for a more user-friendly interface.
     * @param {string} prompt The prompt input.
     * @param {LLModelInferenceOptions} options Optional parameters for the generation.
     * @returns {LLModelInferenceResult} The response text and final context size.
     */
    infer(
        prompt: string,
        options: LLModelInferenceOptions
    ): Promise<LLModelInferenceResult>;

    /**
     * Embed text with the model. See EmbeddingOptions for more information.
     * Use the higher level createEmbedding methods for a more user-friendly interface.
     * @param {string} text
     * @param {string} prefix
     * @param {number} dimensionality
     * @param {boolean} doMean
     * @param {boolean} atlas
     * @returns {Float32Array} The embedding of the text.
     */
    embed(
        text: string,
        prefix: string,
        dimensionality: number,
        doMean: boolean,
        atlas: boolean
    ): Float32Array;

    /**
     * Embed multiple texts with the model. See EmbeddingOptions for more information.
     * Use the higher level createEmbedding methods for a more user-friendly interface.
     * @param {string[]} texts
     * @param {string} prefix
     * @param {number} dimensionality
     * @param {boolean} doMean
     * @param {boolean} atlas
     * @returns {Float32Array[]} The embeddings of the texts.
     */
    embed(
        texts: string,
        prefix: string,
        dimensionality: number,
        doMean: boolean,
        atlas: boolean
    ): Float32Array[];

    /**
     * Whether the model is loaded or not.
     */
    isModelLoaded(): boolean;

    /**
     * Where to search for the pluggable backend libraries
     */
    setLibraryPath(s: string): void;

    /**
     * Where to get the pluggable backend libraries
     */
    getLibraryPath(): string;
    
    /**
     * Get the name of the GPU device that is being used.
     */
    getGpuDeviceName(): ?string;
    
    /**
     * Get the name of the backend that is being used.
     */
    getBackendName(): string;

    /**
     * GPUs that are usable for this LLModel
     * @throws if gpu device list is not available
     * @returns an array of GpuDevice objects
     */
    getGpuDevices(): GpuDevice[];

    /**
     * delete and cleanup the native model
     */
    dispose(): void;
}
/**
 * an object that contains gpu data on this machine.
 */
interface GpuDevice {
    index: number;
    /**
     * same as VkPhysicalDeviceType
     */
    type: number;
    heapSize: number;
    name: string;
    vendor: string;
    backend: string;
}

/**
 * Options that configure a model's behavior.
 */
interface LoadModelOptions {
    /**
     * Where to look for model files.
     */
    modelPath?: string;
    /**
     * Where to look for the backend libraries.
     */
    librariesPath?: string;
    /**
     * The path to the model configuration file, useful for offline usage or custom model configurations.
     */
    modelConfigFile?: string;
    /**
     * Whether to allow downloading the model if it is not present at the specified path.
     */
    allowDownload?: boolean;
    /**
     * Enable verbose logging.
     */
    verbose?: boolean;
    /**
     * The processing unit on which the model will run. It can be set to
     * - "cpu": Model will run on the central processing unit.
     * - "kompute": Model will run using the kompute (vulkan) gpu backend
     * - "cuda": Model will run using the cuda gpu backend
     * - "gpu": Use Metal on ARM64 macOS, otherwise the same as "kompute"
     * - "amd", "nvidia":  Use the best GPU provided by the Kompute backend from this vendor.
     * - "gpu name": Model will run on the GPU that matches the name if it's available.
     * Note: If a GPU device lacks sufficient RAM to accommodate the model, an error will be thrown, and the GPT4All
     * instance will be rendered invalid. It's advised to ensure the device has enough memory before initiating the
     * model.
     * @default Metal on ARM64 macOS, "cpu" otherwise.
     */
    device?: string;
    /**
     * The Maximum window size of this model
     * @default 2048
     */
    nCtx?: number;
    /**
     * Number of GPU layers to use (Vulkan)
     * @default 100
     * @alias ngl
     */
    nGpuLayers?: number;
    ngl?: number;
    /**
     * Number of CPU threads used by GPT4All. Default is None, then the number of threads are determined automatically.
     */
    nThreads?: number;
}

interface InferenceModelOptions extends LoadModelOptions {
    type?: "inference";
}

interface EmbeddingModelOptions extends LoadModelOptions {
    type: "embedding";
}

/**
 * Loads a machine learning model with the specified name. The defacto way to create a model.
 * By default this will download a model from the official GPT4ALL website, if a model is not present at given path.
 *
 * @param {string} modelName - The name of the model to load.
 * @param {LoadModelOptions|undefined} [options] - (Optional) Additional options for loading the model.
 * @returns {Promise<InferenceModel | EmbeddingModel>} A promise that resolves to an instance of the loaded LLModel.
 */
declare function loadModel(
    modelName: string,
    options?: InferenceModelOptions
): Promise<InferenceModel>;

declare function loadModel(
    modelName: string,
    options?: EmbeddingModelOptions
): Promise<EmbeddingModel>;

declare function loadModel(
    modelName: string,
    options?: EmbeddingModelOptions | InferenceModelOptions
): Promise<InferenceModel | EmbeddingModel>;

/**
 * Interface for createCompletion methods, implemented by InferenceModel and ChatSession.
 * Implement your own CompletionProvider or extend ChatSession to generate completions with custom logic.
 */
interface CompletionProvider {
    modelName: string;
    generate(
        input: CompletionInput,
        options?: CompletionOptions
    ): Promise<InferenceResult>;
}

interface CompletionTokens {
    /** The token ids. */
    tokenIds: number[];
    /** The token text. May be an empty string. */
    text: string;
}

/**
 * Options for creating a completion.
 */
interface CompletionOptions extends Partial<LLModelPromptContext> {
    /**
     * Indicates if verbose logging is enabled.
     * @default false
     */
    verbose?: boolean;

    /** Called every time new tokens can be decoded to text.
     * @param {CompletionTokens} tokens The token ids and decoded text.
     * @returns {boolean | undefined} Whether to continue generating tokens.
     * */
    onResponseTokens?: (tokens: CompletionTokens) => boolean | void;
    /** Callback for prompt tokens, called for each input token in the prompt.
     * @param {number} tokenId The token id.
     * @returns {boolean | undefined} Whether to continue ingesting the prompt.
     * */
    onPromptToken?: (tokenId: number) => boolean | void;
}

/**
 * The input for creating a completion. May be a string or an array of messages.
 */
type CompletionInput = string | ChatMessage[];

/**
 * The nodejs equivalent to python binding's chat_completion
 * @param {CompletionProvider} provider - The inference model object or chat session
 * @param {CompletionInput} input - The input string or message array
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {CompletionResult} The completion result.
 */
declare function createCompletion(
    provider: CompletionProvider,
    input: CompletionInput,
    options?: CompletionOptions
): Promise<CompletionResult>;

/**
 * Streaming variant of createCompletion, returns a stream of tokens and a promise that resolves to the completion result.
 * @param {CompletionProvider} provider - The inference model object or chat session
 * @param {CompletionInput} input - The input string or message array
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {CompletionStreamReturn} An object of token stream and the completion result promise.
 */
declare function createCompletionStream(
    provider: CompletionProvider,
    input: CompletionInput,
    options?: CompletionOptions
): CompletionStreamReturn;

/**
 * The result of a streamed completion, containing a stream of tokens and a promise that resolves to the completion result.
 */
interface CompletionStreamReturn {
    tokens: NodeJS.ReadableStream;
    result: Promise<CompletionResult>;
}

/**
 * Async generator variant of createCompletion, yields tokens as they are generated and returns the completion result.
 * @param {CompletionProvider} provider - The inference model object or chat session
 * @param {CompletionInput} input - The input string or message array
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {AsyncGenerator<string>} The stream of generated tokens
 */
declare function createCompletionGenerator(
    provider: CompletionProvider,
    input: CompletionInput,
    options: CompletionOptions
): AsyncGenerator<string, CompletionResult>;

/**
 * A message in the conversation.
 */
interface ChatMessage {
    /** The role of the message. */
    role: "system" | "assistant" | "user";

    /** The message content. */
    content: string;
}

/**
 * The result of a completion.
 */
interface CompletionResult {
    /** The model used for the completion. */
    model: string;

    /** Token usage report. */
    usage: {
        /** The number of tokens ingested during the completion. */
        prompt_tokens: number;

        /** The number of tokens generated in the completion. */
        completion_tokens: number;

        /** The total number of tokens used. */
        total_tokens: number;

        /** Number of tokens used in the conversation. */
        n_past_tokens: number;
    };

    /** The generated completion. */
    choices: Array<{
        message: ChatMessage;
    }>;
}

/**
 * Model inference arguments for generating completions.
 */
interface LLModelPromptContext {
    /** The size of the raw logits vector. */
    logitsSize: number;

    /** The size of the raw tokens vector. */
    tokensSize: number;

    /** The number of tokens in the past conversation.
     * This may be used to "roll back" the conversation to a previous state.
     * Note that for most use cases the default value should be sufficient and this should not be set.
     * @default 0 For completions using InferenceModel, meaning the model will only consider the input prompt.
     * @default nPast For completions using ChatSession. This means the context window will be automatically determined
     * and possibly resized (see contextErase) to keep the conversation performant.
     * */
    nPast: number;

    /** The maximum number of tokens to predict.
     * @default 4096
     * */
    nPredict: number;

    /** Template for user / assistant message pairs.
     * %1 is required and will be replaced by the user input.
     * %2 is optional and will be replaced by the assistant response. If not present, the assistant response will be appended.
     */
    promptTemplate?: string;

    /** The top-k logits to sample from.
     * Top-K sampling selects the next token only from the top K most likely tokens predicted by the model.
     * It helps reduce the risk of generating low-probability or nonsensical tokens, but it may also limit
     * the diversity of the output. A higher value for top-K (eg., 100) will consider more tokens and lead
     * to more diverse text, while a lower value (eg., 10) will focus on the most probable tokens and generate
     * more conservative text. 30 - 60 is a good range for most tasks.
     * @default 40
     * */
    topK: number;

    /** The nucleus sampling probability threshold.
     * Top-P limits the selection of the next token to a subset of tokens with a cumulative probability
     * above a threshold P. This method, also known as nucleus sampling, finds a balance between diversity
     * and quality by considering both token probabilities and the number of tokens available for sampling.
     * When using a higher value for top-P (eg., 0.95), the generated text becomes more diverse.
     * On the other hand, a lower value (eg., 0.1) produces more focused and conservative text.
     * @default 0.9
     *
     * */
    topP: number;

    /**
     * The minimum probability of a token to be considered.
     * @default 0.0
     */
    minP: number;

    /** The temperature to adjust the model's output distribution.
     * Temperature is like a knob that adjusts how creative or focused the output becomes. Higher temperatures
     * (eg., 1.2) increase randomness, resulting in more imaginative and diverse text. Lower temperatures (eg., 0.5)
     * make the output more focused, predictable, and conservative. When the temperature is set to 0, the output
     * becomes completely deterministic, always selecting the most probable next token and producing identical results
     * each time. Try what value fits best for your use case and model.
     * @default 0.1
     * @alias temperature
     * */
    temp: number;
    temperature: number;

    /** The number of predictions to generate in parallel.
     * By splitting the prompt every N tokens, prompt-batch-size reduces RAM usage during processing. However,
     * this can increase the processing time as a trade-off. If the N value is set too low (e.g., 10), long prompts
     * with 500+ tokens will be most affected, requiring numerous processing runs to complete the prompt processing.
     * To ensure optimal performance, setting the prompt-batch-size to 2048 allows processing of all tokens in a single run.
     * @default 8
     * */
    nBatch: number;

    /** The penalty factor for repeated tokens.
     * Repeat-penalty can help penalize tokens based on how frequently they occur in the text, including the input prompt.
     * A token that has already appeared five times is penalized more heavily than a token that has appeared only one time.
     * A value of 1 means that there is no penalty and values larger than 1 discourage repeated tokens.
     * @default 1.18
     * */
    repeatPenalty: number;

    /** The number of last tokens to penalize.
     * The repeat-penalty-tokens N option controls the number of tokens in the history to consider for penalizing repetition.
     * A larger value will look further back in the generated text to prevent repetitions, while a smaller value will only
     * consider recent tokens.
     * @default 10
     * */
    repeatLastN: number;

    /** The percentage of context to erase if the context window is exceeded.
     * Set it to a lower value to keep context for longer at the cost of performance.
     * @default 0.75
     * */
    contextErase: number;
}

/**
 * From python api:
 * models will be stored in (homedir)/.cache/gpt4all/`
 */
declare const DEFAULT_DIRECTORY: string;
/**
 * From python api:
 * The default path for dynamic libraries to be stored.
 * You may separate paths by a semicolon to search in multiple areas.
 * This searches DEFAULT_DIRECTORY/libraries, cwd/libraries, and finally cwd.
 */
declare const DEFAULT_LIBRARIES_DIRECTORY: string;

/**
 * Default model configuration.
 */
declare const DEFAULT_MODEL_CONFIG: ModelConfig;

/**
 * Default prompt context.
 */
declare const DEFAULT_PROMPT_CONTEXT: LLModelPromptContext;

/**
 * Default model list url.
 */
declare const DEFAULT_MODEL_LIST_URL: string;

/**
 * Initiates the download of a model file.
 * By default this downloads without waiting. use the controller returned to alter this behavior.
 * @param {string} modelName - The model to be downloaded.
 * @param {DownloadModelOptions} options - to pass into the downloader. Default is { location: (cwd), verbose: false }.
 * @returns {DownloadController} object that allows controlling the download process.
 *
 * @throws {Error} If the model already exists in the specified location.
 * @throws {Error} If the model cannot be found at the specified url.
 *
 * @example
 * const download = downloadModel('ggml-gpt4all-j-v1.3-groovy.bin')
 * download.promise.then(() => console.log('Downloaded!'))
 */
declare function downloadModel(
    modelName: string,
    options?: DownloadModelOptions
): DownloadController;

/**
 * Options for the model download process.
 */
interface DownloadModelOptions {
    /**
     * location to download the model.
     * Default is process.cwd(), or the current working directory
     */
    modelPath?: string;

    /**
     * Debug mode -- check how long it took to download in seconds
     * @default false
     */
    verbose?: boolean;

    /**
     * Remote download url. Defaults to `https://gpt4all.io/models/gguf/<modelName>`
     * @default https://gpt4all.io/models/gguf/<modelName>
     */
    url?: string;
    /**
     * MD5 sum of the model file. If this is provided, the downloaded file will be checked against this sum.
     * If the sums do not match, an error will be thrown and the file will be deleted.
     */
    md5sum?: string;
}

interface ListModelsOptions {
    url?: string;
    file?: string;
}

declare function listModels(
    options?: ListModelsOptions
): Promise<ModelConfig[]>;

interface RetrieveModelOptions {
    allowDownload?: boolean;
    verbose?: boolean;
    modelPath?: string;
    modelConfigFile?: string;
}

declare function retrieveModel(
    modelName: string,
    options?: RetrieveModelOptions
): Promise<ModelConfig>;

/**
 * Model download controller.
 */
interface DownloadController {
    /** Cancel the request to download if this is called. */
    cancel: () => void;
    /** A promise resolving to the downloaded models config once the download is done */
    promise: Promise<ModelConfig>;
}

export {
    LLModel,
    LLModelPromptContext,
    ModelConfig,
    InferenceModel,
    InferenceResult,
    EmbeddingModel,
    EmbeddingResult,
    ChatSession,
    ChatMessage,
    CompletionInput,
    CompletionProvider,
    CompletionOptions,
    CompletionResult,
    LoadModelOptions,
    DownloadController,
    RetrieveModelOptions,
    DownloadModelOptions,
    GpuDevice,
    loadModel,
    downloadModel,
    retrieveModel,
    listModels,
    createCompletion,
    createCompletionStream,
    createCompletionGenerator,
    createEmbedding,
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_MODEL_CONFIG,
    DEFAULT_PROMPT_CONTEXT,
    DEFAULT_MODEL_LIST_URL,
};
