
#[derive(Debug, Clone)]
pub struct CompletionModelInfo {
    /// Optional field containing the default prompt template used by the model.
    pub prompt_template: Option<String>,
}

/// Struct representing a completion request.
#[derive(Debug, Clone)]
pub struct CompletionRequest {
    /// The prompt string.
    pub prompt: String,
    /// The prompt template string.
    pub prompt_template: String,
    /// Callback function for handling response parts (tokens) to the prompt.
    /// This function will be called for each response part (token) generated.
    /// It takes a single argument `response_part`, which is a reference to the received response part (token) as a string slice (`&str`).
    /// The function should return a boolean indicating whether the generating should continue (`true`) or stop (`false`).
    pub response_callback: fn(&str) -> bool,
    /// Context for completion, containing parameters for generating completions.
    pub context: CompletionContext
}


/// Struct representing the completion context.
#[derive(Debug, Clone)]
pub struct CompletionContext {
    /// Number of tokens in past conversation.
    pub n_past: i32,
    /// Number of tokens to predict.
    pub n_predict: i32,
    /// Top k logits to sample from.
    pub top_k: i32,
    /// Nucleus sampling probability threshold.
    pub top_p: f32,
    /// Min P sampling.
    pub min_p: f32,
    /// Temperature to adjust model's output distribution.
    pub temp: f32,
    /// Number of predictions to generate in parallel.
    pub n_batch: i32,
    /// Penalty factor for repeated tokens.
    pub repeat_penalty: f32,
    /// Last n tokens to penalize.
    pub repeat_last_n: i32,
    /// Percent of context to erase if we exceed the context window.
    pub context_erase: f32,
}

/// Builder for `CompletionContext`.
#[derive(Debug, Clone)]
pub struct CompletionContextBuilder {
    n_past: i32,
    n_predict: i32,
    top_k: i32,
    top_p: f32,
    min_p: f32,
    temp: f32,
    n_batch: i32,
    repeat_penalty: f32,
    repeat_last_n: i32,
    context_erase: f32,
}

impl CompletionContextBuilder {
    /// Creates a new `CompletionContextBuilder` with default values.
    pub fn new() -> Self {
        let default_context = CompletionContext::default();
        Self {
            n_past: default_context.n_past,
            n_predict: default_context.n_predict,
            top_k: default_context.top_k,
            top_p: default_context.top_p,
            min_p: default_context.min_p,
            temp: default_context.temp,
            n_batch: default_context.n_batch,
            repeat_penalty: default_context.repeat_penalty,
            repeat_last_n: default_context.repeat_last_n,
            context_erase: default_context.context_erase,
        }
    }

    /// Sets the number of tokens in past conversation.
    pub fn n_past(mut self, value: i32) -> Self {
        self.n_past = value;
        self
    }

    /// Sets the number of tokens to predict.
    pub fn n_predict(mut self, value: i32) -> Self {
        self.n_predict = value;
        self
    }

    /// Sets the top k logits to sample from.
    pub fn top_k(mut self, value: i32) -> Self {
        self.top_k = value;
        self
    }

    /// Sets the nucleus sampling probability threshold.
    pub fn top_p(mut self, value: f32) -> Self {
        self.top_p = value;
        self
    }

    /// Sets the Min P sampling.
    pub fn min_p(mut self, value: f32) -> Self {
        self.min_p = value;
        self
    }

    /// Sets the temperature to adjust model's output distribution.
    pub fn temp(mut self, value: f32) -> Self {
        self.temp = value;
        self
    }

    /// Sets the number of predictions to generate in parallel.
    pub fn n_batch(mut self, value: i32) -> Self {
        self.n_batch = value;
        self
    }

    /// Sets the penalty factor for repeated tokens.
    pub fn repeat_penalty(mut self, value: f32) -> Self {
        self.repeat_penalty = value;
        self
    }

    /// Sets the last n tokens to penalize.
    pub fn repeat_last_n(mut self, value: i32) -> Self {
        self.repeat_last_n = value;
        self
    }

    /// Sets the percent of context to erase if it exceeds the context window.
    pub fn context_erase(mut self, value: f32) -> Self {
        self.context_erase = value;
        self
    }

    /// Builds the `CompletionContext` instance.
    pub fn build(self) -> CompletionContext {
        CompletionContext {
            n_past: self.n_past,
            n_predict: self.n_predict,
            top_k: self.top_k,
            top_p: self.top_p,
            min_p: self.min_p,
            temp: self.temp,
            n_batch: self.n_batch,
            repeat_penalty: self.repeat_penalty,
            repeat_last_n: self.repeat_last_n,
            context_erase: self.context_erase,
        }
    }
}

/// Struct representing an expectation for completion ( used in prompt engineering ).
#[derive(Debug, Clone)]
pub struct CompletionExpectation {
    /// The main prompt string.
    pub prompt: String,
    /// The prompt template string.
    pub prompt_template: String,
    /// The expected fake reply for the completion.
    pub fake_reply: String,
    /// Number of tokens in past conversation to keep in context.
    pub n_past: i32
}

/// Struct representing a response from a completion request.
pub struct CompletionResponse {
    /// The completed message, if available.
    pub message: Option<String>,
    /// The count of all memoized tokens currently present in context,
    /// including tokens from the latest 'CompletionRequest.prompt' and 'CompletionResponse.message'.
    pub memoized_token_count: i32
}


#[derive(Debug, Clone)]
pub struct CompletionRequestBuilder {
    prompt: String,
    prompt_template: String,
    response_callback: fn(&str) -> bool,
    context: CompletionContext,
}

impl CompletionRequestBuilder {
    /// Creates a new `CompletionRequestBuilder` with default values.
    pub fn new() -> Self {
        let default_request = CompletionRequest::default();
        Self {
            prompt: default_request.prompt,
            prompt_template: default_request.prompt_template,
            response_callback: default_request.response_callback,
            context: default_request.context,
        }
    }

    /// Sets the prompt string.
    pub fn prompt(mut self, value: &str) -> Self {
        self.prompt = value.to_string();
        self
    }

    /// Sets the prompt template string.
    pub fn prompt_template(mut self, value: &str) -> Self {
        self.prompt_template = value.to_string();
        self
    }

    /// Sets the response callback function.
    /// This function will be called for each response part (token) generated.
    /// It takes a single argument `response_part`, which is a reference to the received response part (token) as a string slice (`&str`).
    /// The function should return a boolean indicating whether the generating should continue (`true`) or stop (`false`).
    pub fn response_callback(mut self, value: fn(&str) -> bool) -> Self {
        self.response_callback = value;
        self
    }

    /// Sets the completion context.
    pub fn context(mut self, context: CompletionContext) -> Self {
        self.context = context;
        self
    }

    /// Builds the `CompletionRequest` instance.
    pub fn build(self) -> CompletionRequest {
        CompletionRequest {
            prompt: self.prompt,
            prompt_template: self.prompt_template,
            response_callback: self.response_callback,
            context: self.context,
        }
    }
}


/// Represents a description of the system or environment for contextualizing completion requests.
/// It should follow a specific format or template that aligns with the requirements of the used model.
///
/// # Examples
///
/// Example 1: Mistral OpenOrca
///
/// ```
///
/// let system_description = SystemDescription {
///     system_prompt: "system\nYou are MistralOrca, a large language model trained by Alignment Lab AI.\n\n".to_string(),
/// };
/// ```
///
/// Example 2: Mini Orca (Small)
///
/// ```
///
/// let system_description = SystemDescription {
///     system_prompt: "### System:\nYou are an AI assistant that follows instruction extremely well. Help as much as you can.\n\n".to_string(),
/// };
/// ```
///
#[derive(Debug, Clone)]
pub struct SystemDescription {
    /// The system prompt provides description of the system, following a specific format or template required by the model.
    pub system_prompt: String,
}
