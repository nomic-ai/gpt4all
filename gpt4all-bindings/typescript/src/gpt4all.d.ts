/// <reference types="node" />
declare module 'gpt4all-ts';




interface LLModelPromptContext {
  
  // Size of the raw logits vector
  logits_size: number;
  
  // Size of the raw tokens vector
  tokens_size: number;
  
  // Number of tokens in past conversation
  n_past: number;
  
  // Number of tokens possible in context window
  n_ctx: number;
  
  // Number of tokens to predict
  n_predict: number;
  
  // Top k logits to sample from
  top_k: number;
  
  // Nucleus sampling probability threshold
  top_p: number;
  
  // Temperature to adjust model's output distribution
  temp: number;
  
  // Number of predictions to generate in parallel
  n_batch: number;
  
  // Penalty factor for repeated tokens
  repeat_penalty: number;
  
  // Last n tokens to penalize
  repeat_last_n: number;
  
  // Percent of context to erase if we exceed the context window
  context_erase: number;
}


/**
 * LLModel class representing a language model.
 * This is a base class that provides common functionality for different types of language models.
 */
declare class LLModel {
    //either 'gpt', mpt', or 'llama'
    type() : ModelType;
    //The name of the model
    name(): ModelFile;
    constructor(path: string);
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
    threadCount() : number;
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
    raw_prompt(q: string, params?: Partial<LLModelPromptContext>) : unknown; //todo work on return type
    
}

interface DownloadController {
    //Cancel the request to download from gpt4all website if this is called.
    cancel: () => void;
    //Convert the downloader into a promise, allowing people to await and manage its lifetime
    promise: () => Promise<void>
}


export interface DownloadConfig {
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
     * Default link = https://gpt4all.io/models`
     * This property overrides the default.
     */
    link?: string
}
/**
 * Initiates the download of a model file of a specific model type.
 * By default this downloads without waiting. use the controller returned to alter this behavior.
 * @param {ModelFile[ModelType]} m - The model file to be downloaded.
 * @param {Record<string, unknown>} op - options to pass into the downloader. Default is { location: (cwd), debug: false }.
 * @returns {DownloadController} A DownloadController object that allows controlling the download process.
 */
declare function download(m: ModelFile[ModelType], op: { location: string, debug: boolean, link?:string }): DownloadController 


type ModelType = 'gptj' | 'llama' | 'mpt';

/*
 * A nice interface for intellisense of all possibly models. 
 */
interface ModelFile {
    'gptj': | "ggml-gpt4all-j-v1.3-groovy.bin"
            | "ggml-gpt4all-j-v1.2-jazzy.bin"
            | "ggml-gpt4all-j-v1.1-breezy.bin"
            | "ggml-gpt4all-j.bin";
    'llama':| "ggml-gpt4all-l13b-snoozy.bin"
            | "ggml-vicuna-7b-1.1-q4_2.bin"
            | "ggml-vicuna-13b-1.1-q4_2.bin"
            | "ggml-wizardLM-7B.q4_2.bin"
            | "ggml-stable-vicuna-13B.q4_2.bin"
            | "ggml-nous-gpt4-vicuna-13b.bin"
    'mpt':  | "ggml-mpt-7b-base.bin"
            | "ggml-mpt-7b-chat.bin"
            | "ggml-mpt-7b-instruct.bin"
}

interface ExtendedOptions {
    verbose?: boolean;
    system?: string;
    header?: string;
    prompt: string;
    promptEntries?: Record<string, unknown>
}

type PromptTemplate = (...args: string[]) => string;

declare function createCompletion(
    model: LLModel,
    pt: PromptTemplate,
    options: LLModelPromptContext&ExtendedOptions 
) : string

function prompt(
    strings: TemplateStringsArray
): PromptTemplate


export { LLModel, LLModelPromptContext, ModelType, download, DownloadController, prompt, ExtendedOptions, createCompletion }
