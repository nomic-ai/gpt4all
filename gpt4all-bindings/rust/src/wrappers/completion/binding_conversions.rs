use std::ptr::null_mut;
use crate::bindings::llmodel_prompt_context;

use crate::wrappers::completion::domain::CompletionContext;

// Implements the transformation from "ModelPromptContext" to "llmodel_prompt_context".
//
// The `llmodel_prompt_context` instance is responsible for managing memory allocation and deallocation,
// ensuring that there is only one instance per model. Consequently, asynchronous completions are
// not possible for a single model.
impl From<CompletionContext> for llmodel_prompt_context {
    fn from(prompt_context: CompletionContext) -> Self {
        Self {
            // Backend library sets the following values after the prompt finishes.
            // Values provided from client code have zero impact on the backend side
            // and only act as value receivers.
            logits: null_mut(),
            logits_size: 0,
            tokens: null_mut(),
            tokens_size: 0,

            // DEPRECATED: The context window size. Do not use, as it has no effect.
            // Refer to loadModel options. Use loadModel's nCtx option instead.
            n_ctx: 0,

            // Copying relevant parameters from the LLModelPromptContext
            n_past: prompt_context.n_past,
            n_predict: prompt_context.n_predict,
            top_k: prompt_context.top_k,
            top_p: prompt_context.top_p,
            min_p: prompt_context.min_p,
            temp: prompt_context.temp,
            n_batch: prompt_context.n_batch,
            repeat_penalty: prompt_context.repeat_penalty,
            repeat_last_n: prompt_context.repeat_last_n,
            context_erase: prompt_context.context_erase,
        }
    }
}
