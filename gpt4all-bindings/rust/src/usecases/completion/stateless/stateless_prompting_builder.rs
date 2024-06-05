use crate::usecases::completion::stateless::stateless_prompting::StatelessPrompting;
use crate::wrappers::completion::completion_model::CompletionModel;
use crate::wrappers::completion::domain::{CompletionExpectation, SystemDescription};

/// Builder for constructing stateless prompting instances.
pub struct StatelessPromptingBuilder<'a> {
    /// Reference to the wrapper instance LLCompletionModel used for prompting.
    model: &'a CompletionModel,

    /// Optional description of the system (AI assistant).
    ///
    /// This description will be set before each completion (ask).
    description: Option<SystemDescription>,

    /// Vector of reply expectations.
    ///
    /// These expectations will be set before each completion (ask).
    reply_expectations: Vec<CompletionExpectation>,
}

impl<'a> StatelessPromptingBuilder<'a> {
    /// Creates a new StatelessPromptingBuilder instance.
    ///
    /// # Arguments
    ///
    /// * `model` - Reference to the wrapper LLCompletionModel instance used for prompting.
    pub fn new(model: &'a CompletionModel) -> Self {
        Self {
            model,
            description: None,
            reply_expectations: vec![],
        }
    }

    /// Sets the system description.
    ///
    /// # Arguments
    ///
    /// * `system_description` - Description of the system (AI assistant). This description
    ///                          will be set before each completion (ask).
    ///
    /// Returns a reference to self for method chaining.
    pub fn system_description(mut self, system_description: &str) -> Self {
        self.description = Some(SystemDescription {
            system_prompt: system_description.to_string(),
        });
        self
    }

    /// Adds a reply expectation (an example of the desired completions)
    ///
    /// # Arguments
    ///
    /// * `prompt` - The prompt for which the reply is expected.
    /// * `expected_reply` - The expected reply for the prompt.
    ///
    /// Returns a reference to self for method chaining.
    pub fn add_reply_expectation(mut self, prompt: &str, expected_reply: &str) -> Self {
        self.reply_expectations.push(CompletionExpectation {
            prompt: prompt.to_string(),
            prompt_template: self.model.default_prompt_template(),
            fake_reply: expected_reply.to_string(),
            n_past: 0,
        });
        self
    }

    /// Builds a StatelessPrompting instance.
    ///
    /// Returns the constructed StatelessPrompting instance.
    pub fn build(&self) -> StatelessPrompting {
        StatelessPrompting {
            model: self.model,
            description: self.description.clone(),
            reply_expectations: self.reply_expectations.clone(),
            default_completion_request_builder: self.model.completion_request_builder(),
        }
    }
}
