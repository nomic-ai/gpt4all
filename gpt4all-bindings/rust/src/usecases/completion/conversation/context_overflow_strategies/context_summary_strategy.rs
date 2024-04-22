use crate::usecases::completion::conversation::context_overflow_strategies::strategy::ContextOverflowStrategy;
use crate::usecases::completion::conversation::conversation::Conversation;
use crate::usecases::token_utils::TokenUtils;
use crate::wrappers::completion::domain::{CompletionContextBuilder, CompletionExpectation, CompletionRequest, CompletionRequestBuilder};

/// Summary strategy for managing conversation context overflow by summarizing the conversation.
///
/// The summary strategy is responsible for handling context overflow in a conversation by summarizing the ongoing discussion.
/// It determines if the conversation context will overflow based on the completion request, and if overflow is detected,
/// it asks for the current conversation context summary, describes the system (which resets the context), and feeds the summary
/// back into the conversation context.
#[derive(Clone)]
pub struct SummaryStrategy {
    /// The estimated number of tokens for the summary prompt message.
    summary_msg_token_count: i32,
    /// The maximum number of tokens to be generated for the summary completion response.
    summary_completion_max_token_count: i32,
    /// The message to ask for the conversation summary.
    ask_summary_message: String,
}

impl SummaryStrategy {
    /// Creates a new instance of the summary strategy.
    ///
    /// # Arguments
    ///
    /// * `ask_summary_message` - The message to ask for the conversation summary.
    /// * `summary_completion_max_token_count` - The maximum number of tokens to be generated for the summary completion response.
    pub fn new(ask_summary_message: &str, summary_completion_max_token_count: i32) -> Self {
        let estimated_prompt_tokens = TokenUtils::estimate_token_count(ask_summary_message) as i32;

        Self {
            summary_msg_token_count: estimated_prompt_tokens,
            summary_completion_max_token_count,
            ask_summary_message: ask_summary_message.to_string()
        }
    }

    /// Determines if the conversation context will overflow based on the completion request.
    ///
    /// # Arguments
    ///
    /// * `conversation` - A mutable reference to the conversation.
    /// * `completion_request` - The completion request.
    ///
    /// Returns `true` if the conversation context will overflow; otherwise, `false`.
    fn will_overflow(&self, conversation: &mut Conversation<'_, Self>, completion_request: &CompletionRequest) -> bool {
        let estimated_prompt_tokens = TokenUtils::estimate_token_count(&completion_request.prompt) as i32;

        let free_tokens_count = conversation.model.get_max_context_size() - (conversation.memoized_tokens_count + estimated_prompt_tokens);

        let reserved_token_count = self.summary_msg_token_count + self.summary_completion_max_token_count;

        return free_tokens_count < reserved_token_count;
    }
}


impl Default for SummaryStrategy {
    /// Creates a default instance of the summary strategy.
    ///
    /// The default summary message is "Summarize our conversation, topic and 4 keywords, provide a single and short question/answer sample KEEPING STYLE. YOU MUST FIT MAX 50 words".
    fn default() -> Self {
        let default_summary_msg = "Summarize our conversation, topic and 4 keywords, provide a single and short question/answer sample KEEPING STYLE. YOU MUST FIT MAX 50 words";

        let completion_token_count = TokenUtils::word_to_token_count(50) * 2; // double for reserve

        let summary_msg_token_count = TokenUtils::estimate_token_count(default_summary_msg);

        Self {
            summary_msg_token_count: summary_msg_token_count as i32,
            summary_completion_max_token_count: completion_token_count as i32,
            ask_summary_message: default_summary_msg.to_string()
        }
    }
}

impl ContextOverflowStrategy for SummaryStrategy {
    /// Applies the summary strategy if the conversation context overflows.
    ///
    /// # Arguments
    ///
    /// * `conversation` - A mutable reference to the conversation.
    /// * `completion_request` - The completion request.
    fn apply_if_overflows(&self, conversation: &mut Conversation<'_, Self>, completion_request: &CompletionRequest) {
        if ! self.will_overflow(conversation, completion_request) { return }

        // STEP 1: ASK FOR THE CURRENT CONVERSATION CONTEXT SUMMARY
        let summary = conversation.create_completion(
            CompletionRequestBuilder::new()
                .prompt(&self.ask_summary_message)
                .context(CompletionContextBuilder::new()
                    .n_predict(self.summary_completion_max_token_count)
                    .build())
                .build()
        );

        // STEP 2: DESCRIBE SYSTEM (SYSTEM DESCRIPTION RESETS THE CONTEXT)
        conversation.describe_system();

        // STEP 3: FEED WITH SUMMARY
        let response = conversation.model.provide_completion_expectation(CompletionExpectation {
            prompt: "Summary of our previous conversation: ".to_string() + &summary,
            prompt_template: conversation.model.default_prompt_template(),
            fake_reply: "".to_string(),
            n_past: conversation.memoized_tokens_count
        });

        conversation.memoized_tokens_count = response.memoized_token_count;
    }
}
