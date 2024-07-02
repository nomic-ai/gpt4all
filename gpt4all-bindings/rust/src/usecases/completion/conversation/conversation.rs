use crate::usecases::completion::conversation::context_overflow_strategies::strategy::ContextOverflowStrategy;
use crate::wrappers::completion::completion_model::CompletionModel;
use crate::wrappers::completion::domain::{
    CompletionExpectation, CompletionRequest, CompletionRequestBuilder, SystemDescription,
};

/// Represents a conversation context with strategies to handle context overflow.
pub struct Conversation<'a, Strategy: ContextOverflowStrategy> {
    /// Reference to the completion model used for conversation.
    pub(crate) model: &'a CompletionModel,
    /// Optional description of the system (AI assistant).
    ///
    /// This description will be set before each conversation interaction.
    pub(crate) description: Option<SystemDescription>,
    /// Vector of reply expectations for the conversation.
    ///
    /// These expectations will be set before each conversation interaction.
    pub(crate) reply_expectations: Vec<CompletionExpectation>,
    /// Default prompt builder for generating prompts.
    ///
    /// This builder is used in `ask` and `ask_streamed` calls.
    pub(crate) default_prompt_builder: CompletionRequestBuilder,
    /// The number of memoized tokens representing the context size.
    pub(crate) memoized_tokens_count: i32,
    /// The context overflow strategy for handling context overflow.
    pub(crate) context_overflow_strategy: Strategy,
    /// Flag indicating whether the overflow strategy has been applied.
    pub(crate) is_overflow_strategy_applied: bool,
}

impl<'a, Strategy: ContextOverflowStrategy> Conversation<'_, Strategy> {
    /// Generates a completion for the given message using the default prompt builder of the conversation.
    ///
    /// # Arguments
    ///
    /// * `message` - The prompt message.
    ///
    /// Returns the completion response as a string.
    pub fn ask(&mut self, message: &str) -> String {
        let prompt = self.default_prompt_builder.clone().prompt(message).build();

        self.create_completion(prompt)
    }

    /// Generates a completion for the given message with streaming support using the default prompt builder of the conversation.
    ///
    /// # Arguments
    ///
    /// * `message` - The prompt message.
    /// * `response_callback` - A callback function for handling streamed responses.
    ///   It takes a generated token (`&str`) as input and returns a boolean indicating
    ///   whether to continue generating (`true`) or stop (`false`).
    ///
    /// Returns the completion response as a string.
    pub fn ask_streamed(&mut self, message: &str, response_callback: fn(&str) -> bool) -> String {
        let prompt = self
            .default_prompt_builder
            .clone()
            .prompt(message)
            .response_callback(response_callback)
            .build();

        self.create_completion(prompt)
    }

    /// Describes the system.
    ///
    /// Calling this function will reset the conversation's current context.
    pub fn describe_system(&mut self) {
        let description = self.description.clone();
        self.memoized_tokens_count = if let Some(description) = description {
            self.model.describe_system(description).memoized_token_count
        } else {
            0
        };
    }

    /// Performs initial model training for the conversation, which includes describing the system and providing context with reply expectations.
    ///
    /// Calling this function will reset the conversation's current context.
    pub fn initial_model_train(&mut self) {
        self.describe_system();

        for reply_expectation in &self.reply_expectations {
            self.memoized_tokens_count = self
                .model
                .provide_completion_expectation(CompletionExpectation {
                    n_past: self.memoized_tokens_count,
                    ..reply_expectation.clone()
                })
                .memoized_token_count;
        }
    }

    /// Forgets (resets) the conversation context.
    pub fn forget_conversation_context(&mut self) {
        self.memoized_tokens_count = 0;
    }

    /// Creates a completion based on the provided completion request.
    ///
    /// # Arguments
    ///
    /// * `completion_request` - The completion request.
    ///
    /// Returns the completion response as a string.
    pub fn create_completion(&mut self, completion_request: CompletionRequest) -> String {
        // Apply overflow strategy (iff it is not currently being applied)
        if !self.is_overflow_strategy_applied {
            self.is_overflow_strategy_applied = true;

            self.context_overflow_strategy
                .clone()
                .apply_if_overflows(self, &completion_request);

            self.is_overflow_strategy_applied = false;
        }

        // make a completion call
        let mut completion_request = completion_request;
        let prev_memoized_tokens_count = self.memoized_tokens_count;

        completion_request.context.n_past = self.memoized_tokens_count;
        let completion_response = self.model.create_completion(completion_request);
        self.memoized_tokens_count = completion_response.memoized_token_count;

        completion_response.message.unwrap_or_default()
    }
}
