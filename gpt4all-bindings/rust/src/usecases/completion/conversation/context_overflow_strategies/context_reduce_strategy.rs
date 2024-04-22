use crate::wrappers::completion::domain::CompletionRequest;
use crate::usecases::completion::conversation::conversation::Conversation;
use crate::usecases::completion::conversation::context_overflow_strategies::strategy::ContextOverflowStrategy;

/// "Sliding Window Strategy"
/// A strategy for reducing context by erasing a percentage of the context when it exceeds the context size.
#[derive(Clone)]
pub struct ReduceContextStrategy {}
impl ReduceContextStrategy { pub fn new() -> Self { Self { } } }
impl ContextOverflowStrategy for ReduceContextStrategy {
    /// Applies the strategy if the context overflows for current 'completion_request'.
    ///
    /// Since the reduction is handled by underlying layers, this method is empty.
    ///
    /// # Arguments
    ///
    /// * `conversation` - A mutable reference to the Conversation instance.
    /// * `completion_request` - The CompletionRequest triggering the overflow.
    fn apply_if_overflows(&self, conversation: &mut Conversation<'_, Self>, completion_request: &CompletionRequest) {}
}
