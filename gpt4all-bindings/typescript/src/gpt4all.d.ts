/// <reference types="node" />
declare module 'gpt4all'

/**
 * Initiates the download of a model file of a specific model type.
 * By default this downloads without waiting. use the controller returned to alter this behavior.
 * @param {ModelFile} model - The model file to be downloaded.
 * @param {DownloadOptions} options - to pass into the downloader. Default is { location: (cwd), debug: false }.
 * @returns {DownloadController} object that allows controlling the download process.
 * @example
 * const controller = download('ggml-gpt4all-j-v1.3-groovy.bin')
 * controller.promise().then(() => console.log('Downloaded!'))
 */
declare function download(model: ModelFile[ModelType], options: DownloadOptions): DownloadController

/**
 * Options for the model download process.
 */
export interface DownloadOptions {
    /**
     * location to download the model.
     * Default is process.cwd(), or the current working directory
     */
    location: string;

    /**
     * Debug mode -- check how long it took to download in seconds
     */
    debug: boolean;

    /**
     * Default url = https://gpt4all.io/models
     * This property overrides the default.
     */
    url?: string;
}

/**
 * Model download controller.
 */
interface DownloadController {
    /** Cancel the request to download from gpt4all website if this is called. */
    cancel: () => void;
    /** Convert the downloader into a promise, allowing people to await and manage its lifetime */
    promise: () => Promise<void>;
}

/** Type of the model */
type ModelType = 'gptj' | 'llama' | 'mpt';

/**
 * Full list of models available
 */
interface ModelFile {
    /** List of GPT-J Models */
    gptj: | "ggml-gpt4all-j-v1.3-groovy.bin"
          | "ggml-gpt4all-j-v1.2-jazzy.bin"
          | "ggml-gpt4all-j-v1.1-breezy.bin"
          | "ggml-gpt4all-j.bin";
    /** List Llama Models */
    llama:| "ggml-gpt4all-l13b-snoozy.bin"
          | "ggml-vicuna-7b-1.1-q4_2.bin"
          | "ggml-vicuna-13b-1.1-q4_2.bin"
          | "ggml-wizardLM-7B.q4_2.bin"
          | "ggml-stable-vicuna-13B.q4_2.bin"
          | "ggml-nous-gpt4-vicuna-13b.bin";
    /** List of MPT Models */
    mpt:  | "ggml-mpt-7b-base.bin"
          | "ggml-mpt-7b-chat.bin"
          | "ggml-mpt-7b-instruct.bin";
}

/**
 * LLModel class representing a language model.
 * This is a base class that provides common functionality for different types of language models.
 */
declare class LLModel {

    constructor(path: string)

    /** either 'gpt', mpt', or 'llama' */
    type() : ModelType;

    /** The name of the model. */
    name(): ModelFile

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
      * This is the raw output from std out.
      * Use the prompt function exported for a value 
      * @param q The prompt input.
      * @param params Optional parameters for the prompt context.
      * @returns The result of the model prompt.
     */
    raw_prompt(q: string, params?: Partial<LLModelPromptContext>): unknown; // TODO work on return type

    /**
     * Whether the model is loaded or not.
     */
    isModelLoaded(): boolean;
}

/**
 * The nodejs equivalent to python binding's chat_completion
 * @param {LLModel} llmodel - The language model object.
 * @param {PromptMessage[]} messages - The array of messages for the conversation.
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {CompletionReturn} - The completion result.
 * @example
 * const llmodel = new LLModel(model)
 * const messages = [ 
 * { role: 'system', message: 'You are a weather forecaster.' },
 * { role: 'user', message: 'should i go out today?' } ]
 * const completion = await createCompletion(llmodel, messages, {
 *  verbose: true,
 *  temp: 0.9,
 * })
 * console.log(completion.choices[0].message.content)
 * // No, it's going to be cold and rainy.
  */
declare function createCompletion(
    llmodel: LLModel,
    messages: PromptMessage[],
    options: CompletionOptions
): Promise<CompletionReturn>;

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
     * Indicates if the default header is included in the prompt.
     * @default true
     */
    hasDefaultHeader?: boolean;
    
    /**
     * Indicates if the default footer is included in the prompt.
     * @default true
     */
    hasDefaultFooter?: boolean;
}

/**
 * A message in the conversation, identical to OpenAI's chat message.
 */
interface PromptMessage {
    /** The role of the message. */
    role: "system" | "assistant" | "user";
    
    /** The message content. */
    message: string;
}
/** Token usage report. */
interface ExtendedOptions {
    verbose?: boolean;
    hasDefaultHeader?: boolean,
    hasDefaultFooter?: boolean,
}
/**
 * The result of the completion, similar to OpenAI's format.
 */
interface CompletionReturn {
    /** The model name.
     * @type {ModelFile}
     */
    model : ModelFile[ModelType];
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
    message: {
        /** The role of the message. */
        role: 'assistant';

        /** The message content. */
        content: string;
    };
}

/**
 * Model inference arguments for generating completions.
 */
interface LLModelPromptContext {
    /** The size of the raw logits vector. */
    logits_size: number;
    
    /** The size of the raw tokens vector. */
    tokens_size: number;
    
    /** The number of tokens in the past conversation. */
    n_past: number;
    
    /** The number of tokens possible in the context window. */
    n_ctx: number;
    
    /** The number of tokens to predict. */
    n_predict: number;
    
    /** The top-k logits to sample from. */
    top_k: number;
    
    /** The nucleus sampling probability threshold. */
    top_p: number;
    
    /** The temperature to adjust the model's output distribution. */
    temp: number;
    
    /** The number of predictions to generate in parallel. */
    n_batch: number;
    
    /** The penalty factor for repeated tokens. */
    repeat_penalty: number;
    
    /** The number of last tokens to penalize. */
    repeat_last_n: number;
    
    /** The percentage of context to erase if the context window is exceeded. */
    context_erase: number;
}

/**
  * TODO: Help wanted to implement this
  */
declare function createTokenStream(
    llmodel: LLModel,
    messages: PromptMessage[],
    options: CompletionOptions
): (ll: LLModel) => AsyncGenerator<string>;

interface PromptMessage { 
    role: "system" |"assistant" | "user";
    content: string;
}
export {
    ModelType,
    ModelFile,
    download,
    DownloadController,
    LLModel,
    LLModelPromptContext,
    PromptMessage,
    CompletionOptions,
    createCompletion,
    createTokenStream,
};



