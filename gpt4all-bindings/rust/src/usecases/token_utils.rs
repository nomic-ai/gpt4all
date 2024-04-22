use std::cmp::max;

/// Utility functions for working with text tokenization.
pub struct TokenUtils;

impl TokenUtils {
    /// Estimates the token count based on the input text.
    ///
    /// This function provides a basic estimation of the token count.
    ///
    /// # Arguments
    ///
    /// * `text` - The input text for which the token count is estimated.
    ///
    /// # Returns
    ///
    /// The estimated token count
    pub fn estimate_token_count(text: &str) -> usize {
        // Count the number of words in the text
        let words = text.split_whitespace().count();

        // Estimate token count based on word count and text length
        let estimate_one = Self::word_to_token_count(words);
        let estimate_two = text.len() / 4; // 1 token is cca 4 chars in English

        // Return the maximum of the two estimates
        max(estimate_one, estimate_two)
    }

    /// 1 token is cca (3/4) words
    /// see: https://help.openai.com/en/articles/4936856-what-are-tokens-and-how-to-count-them
    pub fn word_to_token_count(word_count: usize) -> usize {
        (word_count * 4) / 3
    }
}
