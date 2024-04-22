use crate::wrappers::completion::completion_model::CompletionModel;
use crate::wrappers::completion::domain::{CompletionRequest, CompletionExpectation, CompletionRequestBuilder, SystemDescription};


/// A struct for stateless prompting operations.
///
/// Stateless prompting implies that the context of the model will be reset on every completion.
pub struct StatelessPrompting<'a> {
    /// Reference to the wrapper instance LLCompletionModel used for prompting.
    pub(crate) model: &'a CompletionModel,

    /// Optional description of the system (AI assistant).
    ///
    /// This description will be set before each completion (ask).
    pub(crate) description: Option<SystemDescription>,

    /// Vector of reply expectations.
    ///
    /// These expectations will be set before each completion (ask).
    pub(crate) reply_expectations: Vec<CompletionExpectation>,

    /// Default completion request builder.
    ///
    /// This builder is used in `ask` and `ask_streamed` calls.
    pub(crate) default_completion_request_builder: CompletionRequestBuilder
}


impl<'a> StatelessPrompting<'a> {
    /// Generates a completion for the given message.
    ///
    /// # Arguments
    ///
    /// * `message` - The prompt message.
    ///
    /// Returns the completion response as a string.
    pub fn ask(&self, message: &str) -> String {
        let prompt = self.default_completion_request_builder
            .clone()
            .prompt(message)
            .build();

        self.create_completion(prompt)
    }

    /// Generates a completion for the given message with streaming support.
    ///
    /// # Arguments
    ///
    /// * `message` - The prompt message.
    /// * `response_callback` - A callback function for handling streamed responses.
    ///   It takes a generated token (`&str`) as input and returns a boolean indicating
    ///   whether to continue generating (`true`) or stop (`false`).
    ///
    /// Returns the completion response as a string.
    pub fn ask_streamed(&self, message: &str, response_callback: fn(&str) -> bool) -> String {
        let prompt = self.default_completion_request_builder
            .clone()
            .prompt(message)
            .response_callback(response_callback)
            .build();

        self.create_completion(prompt)
    }

    /// Creates a completion based on the provided completion_request.
    ///
    /// # Arguments
    ///
    /// * `completion_request` - The completion request.
    ///
    /// Returns the completion response as a string.
    pub fn create_completion(&self, completion_request: CompletionRequest) -> String {
        let mut context_tokens_count;

        // Describe the system before every completion request
        let description = self.description.clone();
        context_tokens_count = if let Some(description) = description {
            self.model.describe_system(description).memoized_token_count
        } else { 0 };

        // Provide reply expectations before every completion request
        for reply_expectation in &self.reply_expectations {
            context_tokens_count = self.model
                .provide_completion_expectation(CompletionExpectation {
                    n_past: context_tokens_count,
                    ..reply_expectation.clone()
                })
                .memoized_token_count;
        }

        // Make completion call
        let mut completion_request = completion_request;
        completion_request.context.n_past = context_tokens_count;
        let completion_response = self.model.create_completion(completion_request);

        completion_response.message.unwrap_or_default()
    }
}
