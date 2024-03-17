/// <reference types="node" />
declare module "gpt4all";

interface LLModelOptions {
    /**
     * Model architecture. This argument currently does not have any functionality and is just used as descriptive identifier for user.
     */
    type?: string;
    model_name: string;
    model_path: string;
    library_path?: string;
}

interface ModelConfig {
    systemPrompt: string;
    promptTemplate: string;
    path: string;
    url?: string;
}

/**
 * Callback for controlling token generation. Return false to stop token generation.
 */
type TokenCallback = (tokenId: number, token: string, total: string) => boolean;

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
    messages?: Message[];
}

declare class ChatSession {
    constructor(model: InferenceModel, options: ChatSessionOptions);
    model: InferenceModel;
    modelName: string;
    messages: Message[];
    systemPrompt: string;
    promptTemplate: string;

    /**
     * Ingests system prompt and initial messages.
     * Sets this chat session as the active chat session of the model.
     * @param options The options for the chat session.
     */
    initialize(options: ChatSessionOptions): Promise<void>;

    /**
     * Prompts the model in chat-session context.
     * @param prompt The prompt input.
     * @param options Prompt context and other options.
     * @param callback Token generation callback.
     * @returns The model's response to the prompt.
     * @throws {Error} If the chat session is not the active chat session of the model.
     */
    generate(
        prompt: string,
        options?: CompletionOptions,
        callback?: TokenCallback
    ): Promise<CompletionReturn>;
}

/**
 * InferenceModel represents an LLM which can make chat predictions, similar to GPT transformers.
 */
declare class InferenceModel {
    constructor(llm: LLModel, config: ModelConfig);
    llm: LLModel;
    config: ModelConfig;
    activeChatSession?: ChatSession;
    modelName: string;
    nPast: number;

    /**
     * Create a chat session with the model.
     * @param options The options for the chat session.
     * @returns The chat session.
     */
    createChatSession(options?: ChatSessionOptions): Promise<ChatSession>;

    /**
     * Prompts the model with a given input and optional parameters.
     * @param input The prompt input.
     * @param options Prompt context and other options.
     * @param callback Token generation callback.
     * @returns The model's response to the prompt.
     */
    generate(
        prompt: string,
        options?: CompletionOptions,
        callback?: TokenCallback
    ): Promise<CompletionReturn>;

    /**
     * delete and cleanup the native model
     */
    dispose(): void;
}


/**
 * EmbeddingModel represents an LLM which can create embeddings, which are float arrays
 */
declare class EmbeddingModel {
    constructor(llm: LLModel, config: ModelConfig);
    llm: LLModel;
    config: ModelConfig;

    embed(text: string): Float32Array;

    /**
     * delete and cleanup the native model
     */
    dispose(): void;
}

/**
 * Shape of LLModel's inference result.
 */
interface InferenceResult {
    text: string;
    nPast: number;
}

/**
 * LLModel class representing a language model.
 * This is a base class that provides common functionality for different types of language models.
 */
declare class LLModel {
    /**
     * Initialize a new LLModel.
     * @param path Absolute path to the model file.
     * @throws {Error} If the model file does not exist.
     */
    constructor(path: string);
    constructor(options: LLModelOptions);

    /** undefined or user supplied */
    type(): string|undefined;

    /** The name of the model. */
    name(): string;

    /**
     * Get the size of the internal state of the model.
     * NOTE: This state data is specific to the type of model you have created.
     * @return the size in bytes of the internal state of the model
     */
    stateSize(): number;

    /**
     * Get the number of threads used for model inference.
     * The default is the number of physical cores your computer has.
     * @returns The number of threads used for model inference.
     */
    threadCount(): number;

    /**
     * Set the number of threads used for model inference.
     * @param newNumber The new number of threads.
     */
    setThreadCount(newNumber: number): void;

    /**
     * Prompt the model with a given input and optional parameters.
     * This is the raw output from model.
     * Use the prompt function exported for a value
     * @param prompt The prompt input.
     * @param promptContext Optional parameters for the prompt context.
     * @param callback - optional callback to control token generation.
     * @returns The result of the model prompt.
     */
    infer(
        prompt: string,
        promptContext: Partial<LLModelPromptContext>,
        callback?: TokenCallback
    ): Promise<InferenceResult>;

    /**
     * Embed text with the model. Keep in mind that
     * Use the prompt function exported for a value
     * @param text The prompt input.
     * @returns The result of the model prompt.
     */
    embed(text: string): Float32Array;

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
     * Initiate a GPU by a string identifier.
     * @param {number} memory_required Should be in the range size_t or will throw
     * @param {string} device_name  'amd' | 'nvidia' | 'intel' | 'gpu' | gpu name.
     * read LoadModelOptions.device for more information
     */
    initGpuByString(memory_required: number, device_name: string): boolean;

    /**
     * From C documentation
     * @returns True if a GPU device is successfully initialized, false otherwise.
     */
    hasGpuDevice(): boolean;

    /**
     * GPUs that are usable for this LLModel
     * @param nCtx Maximum size of context window
     * @throws if hasGpuDevice returns false (i think)
     * @returns
     */
    listGpu(nCtx: number): GpuDevice[];

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
    * - "gpu": Model will run on the best available graphics processing unit, irrespective of its vendor.
    * - "amd", "nvidia", "intel": Model will run on the best available GPU from the specified vendor.
    * - "gpu name": Model will run on the GPU that matches the name if it's available.
    * Note: If a GPU device lacks sufficient RAM to accommodate the model, an error will be thrown, and the GPT4All
    * instance will be rendered invalid. It's advised to ensure the device has enough memory before initiating the
    * model.
    * @default "cpu"
    */
    device?: string;
    /**
     * The Maximum window size of this model
     * @default 2048
     */
    nCtx?: number;
    /**
     * Number of gpu layers needed
     * @default 100
     */
    ngl?: number;
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
 * Interface for inference, implemented by InferenceModel and ChatSession.
 */
interface InferenceProvider {
    modelName: string;
    generate(
        input: string,
        options?: CompletionOptions,
        callback?: TokenCallback
    ): Promise<CompletionReturn>;
}

/**
 * The nodejs equivalent to python binding's chat_completion
 * @param {InferenceProvider} provider - The inference model object or chat session
 * @param {string} message - The user input message
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {CompletionReturn} The completion result.
 */
declare function createCompletion(
    provider: InferenceProvider,
    message: string,
    options?: CompletionOptions
): Promise<CompletionReturn>;

/**
 * Streaming variant of createCompletion, returns a stream of tokens and a promise that resolves to the completion result.
 * @param {InferenceProvider} provider - The inference model object or chat session
 * @param {string} message - The user input message.
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {CompletionStreamReturn} An object of token stream and the completion result promise.
 */
declare function createCompletionStream(
    provider: InferenceProvider,
    message: string,
    options?: CompletionOptions
): CompletionStreamReturn;

/**
 * Creates an async generator of tokens
 * @param {InferenceProvider} provider - The inference model object or chat session
 * @param {string} message - The user input message.
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {AsyncGenerator<string>} The stream of generated tokens
 */
declare function createCompletionGenerator(
    provider: InferenceProvider,
    message: string,
    options: CompletionOptions
): AsyncGenerator<string, CompletionReturn>;

/**
 * Options for generating one or more embeddings.
 */
interface EmbedddingOptions { 
    /**
     * The model-specific prefix representing the embedding task, without the trailing colon. For Nomic Embed 
     * this can be `search_query`, `search_document`, `classification`, or `clustering`.
     */
    prefix?: string
    /**
     *The embedding dimension, for use with Matryoshka-capable models. Defaults to full-size.
     * @default determines on the model
     */
    dimensionality: number;
    /**
     * How to handle texts longer than the model can accept. One of `mean` or `truncate`.
     * @default "mean"
     */
     long_text_mode?: string
    /**
     * Try to be fully compatible with the Atlas API. Currently, this means texts longer than 8192 tokens 
     * with long_text_mode="mean" will raise an error. Disabled by default.
     * @default false
     */
     atlas?: boolean
}
/**
 * The nodejs moral equivalent to python binding's Embed4All().embed()
 * meow
 * @param {EmbeddingModel} model - The language model object.
 * @param {string} text - text to embed
 * @returns {Float32Array} The completion result.
 */
declare function createEmbedding(
    model: EmbeddingModel,
    text: string,
    options: EmbedddingOptions
): Float32Array;

/**
 * The options for creating the completion.
 */
interface CompletionOptions extends Partial<LLModelPromptContext> {
    /**
     * Indicates if verbose logging is enabled.
     * @default false
     */
    verbose?: boolean;

    /**
     * Callback for controlling token generation. Return false to stop processing.
     */
    onToken?: TokenCallback;
}

/**
 * A message in the conversation.
 */
interface Message {
    /** The role of the message. */
    role: "system" | "assistant" | "user";

    /** The message content. */
    content: string;
}

/**
 * The result of a completion.
 */
interface CompletionReturn {
    /** The model used for the completion. */
    model: string;

    /** Token usage report. */
    usage: {
        /** The number of tokens used in the prompt. Currently not available and always 0. */
        prompt_tokens: number;

        /** The number of tokens used in the completion. */
        completion_tokens: number;

        /** The total number of tokens used. Currently not available and always 0. */
        total_tokens: number;

        /** Number of tokens used in the conversation. */
        n_past_tokens: number;
    };

    /** The generated completion. */
    message: string;
}

/**
 * The result of a streamed completion, containing a stream of tokens and a promise that resolves to the completion result.
 */
interface CompletionStreamReturn {
    tokens: ReadableStream;
    result: Promise<CompletionReturn>;
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
     * This controls how far back the model looks when generating completions.
     * */
    nPast: number;

    /** The maximum number of tokens to predict.
     * @default 4096
     * */
    nPredict: number;
    
    /**
     * Template for user / assistant message pairs.
     * %1 is required and will be replaced by the user input.
     * %2 is optional and will be replaced by the assistant response.
     */
    promptTemplate?: string;

    /** The context window size. Do not use, it has no effect. See loadModel options.
     * THIS IS DEPRECATED!!!
     * Use loadModel's nCtx option instead.
     * @default 2048
     */
    nCtx: number;

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
     * each time. A safe range would be around 0.6 - 0.85, but you are free to search what value fits best for you.
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
    //ModelType,
    ModelConfig,
    InferenceModel,
    InferenceProvider,
    EmbeddingModel,
    ChatSession,
    LLModel,
    LLModelPromptContext,
    Message,
    CompletionOptions,
    CompletionReturn,
    LoadModelOptions,
    loadModel,
    createCompletion,
    createCompletionStream,
    createCompletionGenerator,
    createEmbedding,
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_MODEL_CONFIG,
    DEFAULT_PROMPT_CONTEXT,
    DEFAULT_MODEL_LIST_URL,
    downloadModel,
    retrieveModel,
    listModels,
    DownloadController,
    RetrieveModelOptions,
    DownloadModelOptions,
    GpuDevice,
};
