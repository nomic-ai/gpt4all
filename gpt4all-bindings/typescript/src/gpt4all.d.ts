/// <reference types="node" />
declare module "gpt4all";

/** Type of the model */
type ModelType = "gptj" | "llama" | "mpt" | "replit";

// NOTE: "deprecated" tag in below comment breaks the doc generator https://github.com/documentationjs/documentation/issues/1596
/**
 * Full list of models available
 * @deprecated These model names are outdated and this type will not be maintained, please use a string literal instead
 */
interface ModelFile {
    /** List of GPT-J Models */
    gptj:
        | "ggml-gpt4all-j-v1.3-groovy.bin"
        | "ggml-gpt4all-j-v1.2-jazzy.bin"
        | "ggml-gpt4all-j-v1.1-breezy.bin"
        | "ggml-gpt4all-j.bin";
    /** List Llama Models */
    llama:
        | "ggml-gpt4all-l13b-snoozy.bin"
        | "ggml-vicuna-7b-1.1-q4_2.bin"
        | "ggml-vicuna-13b-1.1-q4_2.bin"
        | "ggml-wizardLM-7B.q4_2.bin"
        | "ggml-stable-vicuna-13B.q4_2.bin"
        | "ggml-nous-gpt4-vicuna-13b.bin"
        | "ggml-v3-13b-hermes-q5_1.bin";
    /** List of MPT Models */
    mpt:
        | "ggml-mpt-7b-base.bin"
        | "ggml-mpt-7b-chat.bin"
        | "ggml-mpt-7b-instruct.bin";
    /** List of Replit Models */
    replit: "ggml-replit-code-v1-3b.bin";
}

//mirrors py options
interface LLModelOptions {
    /**
     * Model architecture. This argument currently does not have any functionality and is just used as descriptive identifier for user.
     */
    type?: ModelType;
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

declare class InferenceModel {
    constructor(llm: LLModel, config: ModelConfig);
    llm: LLModel;
    config: ModelConfig;

    generate(
        prompt: string,
        options?: Partial<LLModelPromptContext>
    ): Promise<string>;
}

declare class EmbeddingModel {
    constructor(llm: LLModel, config: ModelConfig);
    llm: LLModel;
    config: ModelConfig;

    embed(text: string): Float32Array;
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

    /** either 'gpt', mpt', or 'llama' or undefined */
    type(): ModelType | undefined;

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
     * @param q The prompt input.
     * @param params Optional parameters for the prompt context.
     * @returns The result of the model prompt.
     */
    raw_prompt(
        q: string,
        params: Partial<LLModelPromptContext>,
        callback: (res: string) => void
    ): void; // TODO work on return type

    /**
     * Embed text with the model. Keep in mind that
     * not all models can embed text, (only bert can embed as of 07/16/2023 (mm/dd/yyyy))
     * Use the prompt function exported for a value
     * @param q The prompt input.
     * @param params Optional parameters for the prompt context.
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
}

interface LoadModelOptions {
    modelPath?: string;
    librariesPath?: string;
    modelConfigFile?: string;
    allowDownload?: boolean;
    verbose?: boolean;
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
    options?: EmbeddingOptions | InferenceOptions
): Promise<InferenceModel | EmbeddingModel>;

/**
 * The nodejs equivalent to python binding's chat_completion
 * @param {InferenceModel} model - The language model object.
 * @param {PromptMessage[]} messages - The array of messages for the conversation.
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {CompletionReturn} The completion result.
 */
declare function createCompletion(
    model: InferenceModel,
    messages: PromptMessage[],
    options?: CompletionOptions
): Promise<CompletionReturn>;

/**
 * The nodejs moral equivalent to python binding's Embed4All().embed()
 * meow
 * @param {EmbeddingModel} model - The language model object.
 * @param {string} text - text to embed
 * @returns {Float32Array} The completion result.
 */
declare function createEmbedding(
    model: EmbeddingModel,
    text: string
): Float32Array;

/**
 * The options for creating the completion.
 */
interface CompletionOptions extends Partial<LLModelPromptContext> {
    /**
     * Indicates if verbose logging is enabled.
     * @default true
     */
    verbose?: boolean;

    /**
     * Template for the system message. Will be put before the conversation with %1 being replaced by all system messages.
     * Note that if this is not defined, system messages will not be included in the prompt.
     */
    systemPromptTemplate?: string;

    /**
     * Template for user messages, with %1 being replaced by the message.
     */
    promptTemplate?: boolean;

    /**
     * The initial instruction for the model, on top of the prompt
     */
    promptHeader?: string;

    /**
     * The last instruction for the model, appended to the end of the prompt.
     */
    promptFooter?: string;
}

/**
 * A message in the conversation, identical to OpenAI's chat message.
 */
interface PromptMessage {
    /** The role of the message. */
    role: "system" | "assistant" | "user";

    /** The message content. */
    content: string;
}

/**
 * The result of the completion, similar to OpenAI's format.
 */
interface CompletionReturn {
    /** The model used for the completion. */
    model: string;

    /** Token usage report. */
    usage: {
        /** The number of tokens used in the prompt. */
        prompt_tokens: number;

        /** The number of tokens used in the completion. */
        completion_tokens: number;

        /** The total number of tokens used. */
        total_tokens: number;
    };

    /** The generated completions. */
    choices: CompletionChoice[];
}

/**
 * A completion choice, similar to OpenAI's format.
 */
interface CompletionChoice {
    /** Response message */
    message: PromptMessage;
}

/**
 * Model inference arguments for generating completions.
 */
interface LLModelPromptContext {
    /** The size of the raw logits vector. */
    logitsSize: number;

    /** The size of the raw tokens vector. */
    tokensSize: number;

    /** The number of tokens in the past conversation. */
    nPast: number;

    /** The number of tokens possible in the context window.
     * @default 1024
     */
    nCtx: number;

    /** The number of tokens to predict.
     * @default 128
     * */
    nPredict: number;

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
     * The default value is 0.4, which is aimed to be the middle ground between focus and diversity, but
     * for more creative tasks a higher top-p value will be beneficial, about 0.5-0.9 is a good range for that.
     * @default 0.4
     * */
    topP: number;

    /** The temperature to adjust the model's output distribution.
     * Temperature is like a knob that adjusts how creative or focused the output becomes. Higher temperatures
     * (eg., 1.2) increase randomness, resulting in more imaginative and diverse text. Lower temperatures (eg., 0.5)
     * make the output more focused, predictable, and conservative. When the temperature is set to 0, the output
     * becomes completely deterministic, always selecting the most probable next token and producing identical results
     * each time. A safe range would be around 0.6 - 0.85, but you are free to search what value fits best for you.
     * @default 0.7
     * */
    temp: number;

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
     * @default 64
     * */
    repeatLastN: number;

    /** The percentage of context to erase if the context window is exceeded.
     * @default 0.5
     * */
    contextErase: number;
}

/**
 * TODO: Help wanted to implement this
 */
declare function createTokenStream(
    llmodel: LLModel,
    messages: PromptMessage[],
    options: CompletionOptions
): (ll: LLModel) => AsyncGenerator<string>;
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
declare const DEFAULT_PROMT_CONTEXT: LLModelPromptContext;

/**
 * Default model list url.
 */
declare const DEFAULT_MODEL_LIST_URL: string;

/**
 * Initiates the download of a model file.
 * By default this downloads without waiting. use the controller returned to alter this behavior.
 * @param {string} modelName - The model to be downloaded.
 * @param {DownloadOptions} options - to pass into the downloader. Default is { location: (cwd), verbose: false }.
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
     * Remote download url. Defaults to `https://gpt4all.io/models/<modelName>`
     * @default https://gpt4all.io/models/<modelName>
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

declare function listModels(options?: ListModelsOptions): Promise<ModelConfig[]>;

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
    ModelType,
    ModelFile,
    ModelConfig,
    InferenceModel,
    EmbeddingModel,
    LLModel,
    LLModelPromptContext,
    PromptMessage,
    CompletionOptions,
    LoadModelOptions,
    loadModel,
    createCompletion,
    createEmbedding,
    createTokenStream,
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
    DEFAULT_MODEL_CONFIG,
    DEFAULT_PROMT_CONTEXT,
    DEFAULT_MODEL_LIST_URL,
    downloadModel,
    retrieveModel,
    listModels,
    DownloadController,
    RetrieveModelOptions,
    DownloadModelOptions,
};
