use crate::wrappers::completion::domain::CompletionRequest;
use crate::usecases::completion::conversation::conversation::Conversation;


/// Trait for implementing context overflow strategies for a conversation use-case
pub trait ContextOverflowStrategy: Clone {
    /// Applies the strategy if the conversation context for current prompt will overflow.
    ///
    /// # Arguments
    ///
    /// * `conversation` - Mutable reference to the conversation instance.
    /// * `completion_request` - The CompletionRequest containing the prompt data.
    fn apply_if_overflows(&self, conversation: &mut Conversation<'_, Self>, completion_request: &CompletionRequest);
}
