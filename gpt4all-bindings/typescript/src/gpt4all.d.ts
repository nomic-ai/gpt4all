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

    constructor(path: string);
    /**
     * Get the size of the internal state of the model.
     * NOTE: This state data is specific to the type of model you have created.
     * @param model A pointer to the llmodel_model instance.
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
      * @param q The prompt input.
      * @param params Optional parameters for the prompt context.
      * @returns The result of the model prompt.
     */
    prompt(q: string, params?: Partial<LLModelPromptContext>) : unknown; //todo work on return type
    
}

export { LLModel, LLModelPromptContext }
