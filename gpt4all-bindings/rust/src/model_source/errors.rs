use std::error::Error;
use std::fmt::{Display, Formatter};

/// Errors that can occur when interacting with a model source.
#[derive(Debug)]
pub enum ModelSourceError {
    /// Error fetching available models.
    FetchError,
    /// Error parsing model information.
    ModelInfoParsingError,
    /// Error downloading a model.
    ModelDownloadError,
    /// Model not found on the source.
    ModelNotFoundError,
}

impl Error for ModelSourceError {}

impl Display for ModelSourceError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            ModelSourceError::FetchError => f.write_str("Error fetching available models"),
            ModelSourceError::ModelInfoParsingError => {
                f.write_str("Error parsing model information.")
            }
            ModelSourceError::ModelDownloadError => f.write_str("Error downloading a model."),
            ModelSourceError::ModelNotFoundError => f.write_str("Model not found on the source."),
        }
    }
}
