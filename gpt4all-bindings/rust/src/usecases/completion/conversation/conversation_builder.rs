use crate::usecases::completion::conversation::conversation::Conversation;
use crate::usecases::completion::conversation::context_overflow_strategies::strategy::ContextOverflowStrategy;
use crate::wrappers::completion::completion_model::CompletionModel;
use crate::wrappers::completion::domain::{CompletionExpectation, SystemDescription};


/// A builder for creating Conversation instances.
pub struct ConversationBuilder<'a, Strategy: ContextOverflowStrategy> {
    model: &'a CompletionModel,
    description: Option<SystemDescription>,
    reply_expectations: Vec<CompletionExpectation>,
    memoized_tokens_count: i32,
    context_overflow_strategy: Strategy
}


impl<'a, Strategy: ContextOverflowStrategy> ConversationBuilder<'a, Strategy> {
    /// Creates a new ConversationBuilder instance.
    ///
    /// # Arguments
    ///
    /// * `model` - A reference to the CompletionModel used for conversation.
    /// * `context_overflow_strategy` - The strategy to handle context overflow.
    ///
    /// Returns a new ConversationBuilder instance.
    pub fn new(model: &'a CompletionModel, context_overflow_strategy: Strategy) -> Self {
        Self { model, description: None, reply_expectations: vec![], memoized_tokens_count: 0, context_overflow_strategy }
    }

    /// Sets the system description for the conversation.
    ///
    /// # Arguments
    ///
    /// * `system_description` - The description of the system.
    ///
    /// Returns a mutable reference to the ConversationBuilder instance.
    pub fn system_description(mut self, system_description: &str) -> Self {
        self.description = Some(SystemDescription { system_prompt: system_description.to_string() });
        self
    }

    /// Adds a reply expectation to the conversation.
    ///
    /// # Arguments
    ///
    /// * `prompt` - The prompt for the expected reply (completion).
    /// * `expected_reply` - The expected reply (completion).
    ///
    /// Returns a mutable reference to the ConversationBuilder instance.
    pub fn add_completion_expectation(mut self, prompt: &str, expected_reply: &str) -> Self {
        self.reply_expectations.push(CompletionExpectation {
            prompt: prompt.to_string(),
            prompt_template: self.model.default_prompt_template(),
            fake_reply: expected_reply.to_string(),
            n_past: 0,
        });
        self
    }

    /// Builds a Conversation instance based on the provided configurations.
    ///
    /// Returns a new Conversation instance.
    pub fn build(&self) -> Conversation<'a, Strategy> {
        let mut conversation = Conversation {
            model: self.model,
            description: self.description.clone(),
            reply_expectations: self.reply_expectations.clone(),
            default_prompt_builder: self.model.completion_request_builder(),
            memoized_tokens_count: 0,
            context_overflow_strategy: self.context_overflow_strategy.clone(),
            is_overflow_strategy_applied: false,
        };

        conversation.initial_model_train();

        conversation
    }
}
