use crate::wrappers::completion::domain::{CompletionContext, CompletionRequest};

/// The minimum valid prompt template required for completion, represented as `"%1"`.
pub const MIN_VALID_PROMPT_TEMPLATE: &str = "%1";

impl Default for CompletionContext {
    fn default() -> Self {
        Self {
            n_past: 0,
            n_predict: 4096,
            top_k: 40,
            top_p: 0.9,
            min_p: 0.0,
            temp: 0.1,
            n_batch: 8,
            repeat_penalty: 1.2,
            repeat_last_n: 10,
            context_erase: 0.75
        }
    }
}

impl Default for CompletionRequest {
    fn default() -> Self {
        Self {
            prompt: String::default(),
            prompt_template: MIN_VALID_PROMPT_TEMPLATE.to_string(),
            response_callback: |_| { true },
            context: Default::default(),
        }
    }
}
