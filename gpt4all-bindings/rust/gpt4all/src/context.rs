//! Context for prompts.

use gpt4all_sys::llmodel_prompt_context;
use std::slice;
use std::ptr::{NonNull};

/// Obtain a context using [`Context::default`].
#[derive(Debug)]
pub struct Context {
    pub(crate) content: llmodel_prompt_context,
}

impl Context {
    /// Get the logits from the context - will be empty unless passed to [`InitModel::prompt`].
    pub fn logits(&self) -> &[f32] {
        // Safety: we assume that gpt4all has given us a valid pointer to contiguous memory of at
        // least logits_size elements
        unsafe { slice::from_raw_parts(self.content.logits, self.content.logits_size) }
    }
    /// Get the logits from the context - will be empty unless passed to [`InitModel::prompt`].
    pub fn tokens(&self) -> &[i32] {
        // Safety: we assume that gpt4all has given us a valid pointer to contiguous memory of at
        // least tokens_size elements
        unsafe { slice::from_raw_parts(self.content.tokens, self.content.tokens_size) }
    }
    pub fn n_past(&self) -> i32 {
        self.content.n_past
    }
    pub fn n_ctx(&self) -> i32 {
        self.content.n_ctx
    }
    pub fn n_predict(&self) -> i32 {
        self.content.n_predict
    }
    pub fn top_k(&self) -> i32 {
        self.content.top_k
    }
    pub fn top_p(&self) -> f32 {
        self.content.top_p
    }
    pub fn temp(&self) -> f32 {
        self.content.temp
    }
    pub fn n_batch(&self) -> i32 {
        self.content.n_batch
    }
    pub fn repeat_penalty(&self) -> f32 {
        self.content.repeat_penalty
    }
    pub fn repeat_last_n(&self) -> i32 {
        self.content.repeat_last_n
    }
    pub fn context_erase(&self) -> f32 {
        self.content.context_erase
    }
}

impl Default for Context {
    // defaults have been stolen from the python bindings.
    fn default() -> Self {
        Self {
            content: llmodel_prompt_context {
                // The python bindings seem to do this? (not super familiar with python ffi)
                logits: NonNull::dangling().as_ptr(),
                // The python bindings seem to do this? (not super familiar with python ffi)
                tokens: NonNull::dangling().as_ptr(),
                logits_size: 0,
                tokens_size: 0,
                n_past: 0,
                n_ctx: 1024,
                n_predict: 128,
                top_k: 40,
                top_p: 0.9,
                temp: 0.1,
                n_batch: 8,
                repeat_penalty: 1.2,
                repeat_last_n: 10,
                context_erase: 0.5,
            },
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn check_read_empty_logits_and_tokens() {
        let context = Context::default();
        assert_eq!(context.logits(), &[]);
        assert_eq!(context.tokens(), &[]);
    }
}