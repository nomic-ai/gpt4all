use std::{error, fmt};

/// Error type representing various model loading errors.
///
/// `BACKEND_*` errors represent errors propagated from C library calls.
#[derive(Debug)]
pub enum ModelLoadingError {
    /// Model not found in the provided path.
    InvalidModelPath,

    /// Error during model creation with additional message.
    BackendCreateError(String),

    /// Failed to load the model.
    BackendLoadError,

    /// Chosen device can not be used due to insufficient memory or features
    BackendDeviceInitError(String)
}

impl error::Error for ModelLoadingError {}

impl fmt::Display for ModelLoadingError {
    /// Formats the error for display.
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            ModelLoadingError::InvalidModelPath => f.write_str("Model not found in provided path"),
            ModelLoadingError::BackendCreateError(msg) => write!(f, "Model could not be created ( 'llmodel_model_create' failed )\nBackend Error: {}", msg),
            ModelLoadingError::BackendLoadError => f.write_str("Failed to load the model"),
            ModelLoadingError::BackendDeviceInitError(msg) => write!(f, "Device can not be used.\nReason: {}", msg)
        }
    }
}
