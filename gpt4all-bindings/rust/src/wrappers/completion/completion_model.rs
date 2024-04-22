use std::ffi::CString;
use std::ptr::null;

use crate::bindings::{llmodel_model, llmodel_model_destroy, llmodel_prompt, llmodel_prompt_context};
use crate::wrappers::completion::defaults::MIN_VALID_PROMPT_TEMPLATE;
use crate::wrappers::completion::domain::{CompletionModelInfo, CompletionRequest, CompletionContext, CompletionResponse, CompletionExpectation, CompletionRequestBuilder, SystemDescription};
use crate::wrappers::completion::response_provider::ResponseProvider;

/// Struct representing a language model used for completions.
#[derive(Debug)]
pub struct CompletionModel {
    /// The underlying raw model binding.
    raw_model: llmodel_model,
    /// Information about the completion model.
    model_info: CompletionModelInfo,
    /// The maximum size of the context used by the model.
    max_context_size: i32
}


impl CompletionModel {
    /// Creates a new `CompletionModel` instance.
    ///
    /// # Arguments
    ///
    /// * `raw_model` - The underlying raw model.
    /// * `model_info` - Information about the completion model.
    /// * `max_context_size` - The maximum size of the context used by the model.
    ///
    /// Returns the created `CompletionModel`.
    pub fn new(raw_model: llmodel_model,
               model_info: CompletionModelInfo,
               max_context_size: i32) -> Self {
        Self {
            raw_model,
            model_info,
            max_context_size,
        }
    }


    /// Creates a completion based on the provided completion request.
    pub fn create_completion(&self, completion_request: CompletionRequest) -> CompletionResponse {
        let response_provider = ResponseProvider::new();
        response_provider.subscribe_on_response_part(completion_request.response_callback);
        let callbacks = response_provider.get_callbacks();

        let prompt = CString::new(completion_request.prompt).unwrap();
        let prompt_template = CString::new(completion_request.prompt_template).unwrap();

        let prompt_context = &mut llmodel_prompt_context::from(completion_request.context);

        unsafe {
            llmodel_prompt(self.raw_model,
                           prompt.as_ptr(),
                           prompt_template.as_ptr(),
                           callbacks.prompt_callback,
                           callbacks.response_callback,
                           callbacks.recalculate_callback,
                           prompt_context,
                           false,
                           null()
            );
        };

        let response = response_provider.extract_response();

        CompletionResponse { message: Some(response), memoized_token_count: prompt_context.n_past }
    }


    /// Provide a system description to the completion model for contextualizing completion requests.
    /// This description helps the model understand the system or environment in which the completion is requested,
    /// enabling it to generate more relevant and accurate responses.
    pub fn describe_system(&self, system_description: SystemDescription) -> CompletionResponse {
        // Setup response provider and callbacks
        let response_provider = ResponseProvider::new();
        let callbacks = response_provider.get_callbacks();

        // Convert description and prompt template to CStrings
        let description = CString::new(system_description.system_prompt).unwrap();

        // System prompt should have a specific template "%1"
        let prompt_template = CString::new(MIN_VALID_PROMPT_TEMPLATE.to_string()).unwrap();

        // Create and initialize low-level (binding struct) prompt context instance
        let prompt_context = &mut llmodel_prompt_context::from(CompletionContext::default());

        // Setting a system description, we don't need a reply
        let empty_reply = CString::new("".to_string()).unwrap();

        // Call the low-level model prompt backend function
        unsafe {
            llmodel_prompt(self.raw_model,
                           description.as_ptr(),
                           prompt_template.as_ptr(),
                           callbacks.prompt_callback,
                           callbacks.response_callback,
                           callbacks.recalculate_callback,
                           prompt_context,
                           true,
                           empty_reply.as_ptr()
            );
        }

        // Extract response and return CompletionResponse
        CompletionResponse { message: None, memoized_token_count: prompt_context.n_past }
    }

    /// Provide an example of the desired completion to guide the model in generating responses.
    pub fn provide_completion_expectation(&self, completion_expectation: CompletionExpectation) -> CompletionResponse {
        let response_provider = ResponseProvider::new();
        let callbacks = response_provider.get_callbacks();

        let prompt = CString::new(completion_expectation.prompt).unwrap();
        let prompt_template = CString::new(completion_expectation.prompt_template).unwrap();

        let mut prompt_context = CompletionContext::default();
        prompt_context.n_past = completion_expectation.n_past;
        let prompt_context = &mut llmodel_prompt_context::from(prompt_context);

        let expected_reply = CString::new(completion_expectation.fake_reply).unwrap();

        unsafe {
            llmodel_prompt(self.raw_model,
                           prompt.as_ptr(),
                           prompt_template.as_ptr(),
                           callbacks.prompt_callback,
                           callbacks.response_callback,
                           callbacks.recalculate_callback,
                           prompt_context,
                           false,
                            expected_reply.as_ptr()
            );
        };

        CompletionResponse { message: None, memoized_token_count: prompt_context.n_past }
    }

    /// Returns a builder for constructing completion requests (with preset default 'prompt_template')
    pub fn completion_request_builder(&self) -> CompletionRequestBuilder {
        CompletionRequestBuilder::new()
            .prompt_template(&self.default_prompt_template())
    }

    /// Returns the default prompt template (from available 'CompletionModelInfo')
    pub fn default_prompt_template(&self) -> String {
        self.model_info
            .prompt_template
            .clone()
            .unwrap_or(MIN_VALID_PROMPT_TEMPLATE.to_string())
    }

    /// Returns the maximum context size.
    pub fn get_max_context_size(&self) -> i32 {
        self.max_context_size
    }

    /// Disposes of the CompletionModel instance, freeing associated resources.
    pub fn dispose(self) {
        unsafe { llmodel_model_destroy(self.raw_model); }
    }
}
