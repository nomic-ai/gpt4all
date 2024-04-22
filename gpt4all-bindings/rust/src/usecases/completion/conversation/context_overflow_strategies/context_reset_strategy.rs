use crate::wrappers::completion::domain::CompletionRequest;
use crate::usecases::completion::conversation::conversation::Conversation;
use crate::usecases::completion::conversation::context_overflow_strategies::strategy::ContextOverflowStrategy;
use crate::usecases::token_utils::TokenUtils;

/// Strategy for resetting the conversation context when it overflows.
#[derive(Clone)]
pub struct ContextResetStrategy {
    /// The number of tokens reserved to keep the context from overflowing.
    reserved_token_count: i32
}


impl ContextResetStrategy {
    /// Creates a new instance of ContextResetStrategy.
    ///
    /// # Arguments
    ///
    /// * `reserved_token_count` - The number of tokens reserved to keep the context from overflowing.
    pub fn new(reserved_token_count: i32) -> Self { Self { reserved_token_count } }

    /// Determines if the conversation context will overflow based on the completion request.
    fn will_overflow(&self, conversation: &mut Conversation<'_, Self>, completion_request: &CompletionRequest) -> bool {
        let estimated_prompt_tokens = TokenUtils::estimate_token_count(&completion_request.prompt) as i32;

        let free_tokens_count = conversation.model.get_max_context_size() - (conversation.memoized_tokens_count + estimated_prompt_tokens);

        return free_tokens_count < self.reserved_token_count;
    }
}

impl ContextOverflowStrategy for ContextResetStrategy {
    /// Applies the context reset strategy if the conversation context overflows.
    fn apply_if_overflows(&self, conversation: &mut Conversation<'_, Self>, completion_request: &CompletionRequest) {
        if ! self.will_overflow(conversation, completion_request) { return }

        // REINITIALIZE MODEL (system description, completion expectations)
        conversation.initial_model_train();
    }
}
