use std::{error, fmt};
use std::fmt::Formatter;

/// Represents errors that can occur during embedding operations.
///
/// `BACKEND_*` errors represent errors propagated from C library calls.
#[derive(Debug)]
pub enum EmbeddingError {
    /// Indicates that embedding creation failed with the provided error message.
    BackendEmbedFailed(String)
}

impl error::Error for EmbeddingError {}

impl fmt::Display for EmbeddingError {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        match self {
            EmbeddingError::BackendEmbedFailed(message) => write!(f, "{}", message)
        }
    }
}
