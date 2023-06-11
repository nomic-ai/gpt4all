/// <reference types="node" />
declare module "gpt4all";

export * from "./util.d.ts";

/** Type of the model */
type ModelType = "gptj" | "llama" | "mpt" | "replit";

/**
 * Full list of models available
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
    model_name: ModelFile[ModelType];
    model_path: string;
    library_path?: string;
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
    name(): ModelFile;

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
    raw_prompt(q: string, params: Partial<LLModelPromptContext>, callback: (res: string) => void): void; // TODO work on return type

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
    allowDownload?: boolean;
    verbose?: boolean;
}

declare function loadModel(
    modelName: string,
    options?: LoadModelOptions
): Promise<LLModel>;

/**
 * The nodejs equivalent to python binding's chat_completion
 * @param {LLModel} llmodel - The language model object.
 * @param {PromptMessage[]} messages - The array of messages for the conversation.
 * @param {CompletionOptions} options - The options for creating the completion.
 * @returns {CompletionReturn} The completion result.
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
    options?: CompletionOptions
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
    content: string;
}

/**
 * The result of the completion, similar to OpenAI's format.
 */
interface CompletionReturn {
    /** The model name.
     * @type {ModelFile}
     */
    model: ModelFile[ModelType];

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
    logits_size: number;

    /** The size of the raw tokens vector. */
    tokens_size: number;

    /** The number of tokens in the past conversation. */
    n_past: number;

    /** The number of tokens possible in the context window.
     * @default 1024
     */
    n_ctx: number;

    /** The number of tokens to predict.
     * @default 128
     * */
    n_predict: number;

    /** The top-k logits to sample from.
     * @default 40
     * */
    top_k: number;

    /** The nucleus sampling probability threshold.
     * @default 0.9
     * */
    top_p: number;

    /** The temperature to adjust the model's output distribution.
     * @default 0.72
     * */
    temp: number;

    /** The number of predictions to generate in parallel.
     * @default 8
     * */
    n_batch: number;

    /** The penalty factor for repeated tokens.
     * @default 1
     * */
    repeat_penalty: number;

    /** The number of last tokens to penalize.
     * @default 10
     * */
    repeat_last_n: number;

    /** The percentage of context to erase if the context window is exceeded.
     * @default 0.5
     * */
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
interface PromptMessage {
    role: "system" | "assistant" | "user";
    content: string;
}
export {
    ModelType,
    ModelFile,
    LLModel,
    LLModelPromptContext,
    PromptMessage,
    CompletionOptions,
    LoadModelOptions,
    loadModel,
    createCompletion,
    createTokenStream,
    DEFAULT_DIRECTORY,
    DEFAULT_LIBRARIES_DIRECTORY,
};
