
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
    ModelNotFound
}
