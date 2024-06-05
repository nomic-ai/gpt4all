use crate::wrappers::embedding::domain::EmbeddingOptions;

impl Default for EmbeddingOptions {
    /// Returns a new `EmbeddingOptions` with default values.
    fn default() -> Self {
        Self {
            texts: vec![],
            prefix: None,
            dimensionality: -1,
            do_mean: false,
            atlas: false,
        }
    }
}
