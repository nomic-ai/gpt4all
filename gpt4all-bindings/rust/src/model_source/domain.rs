use crate::model_source::errors::ModelSourceError;

/// Trait for fetching models from a remote source.
pub trait ModelSource {
    /// Asynchronously fetches available models from the source.
    ///
    /// Returns a future that resolves to a vector of `ModelInfo` or an error if fetching fails.
    fn fetch_all_models(&self) -> impl std::future::Future<Output = Result<Vec<ModelInfo>, ModelSourceError>> + Send;

    /// Asynchronously downloads a model file.
    ///
    /// # Arguments
    ///
    /// * `model_file_name` - The name of the model file to download.
    ///
    /// Returns a future that resolves to the path of the downloaded file or an error if download fails.
    fn download(&self, model_file_name: &str) -> impl std::future::Future<Output = Result<String, ModelSourceError>> + Send;

    /// Retrieves the names of local models from provided folder (already downloaded from GPT-4-All repository).
    fn get_local_models(&self) -> impl std::future::Future<Output = Result<Vec<String>, ModelSourceError>> + Send;
}

/// Information about a model available for download.
#[derive(Debug)]
pub struct ModelInfo {
    /// The name of the model.
    pub name: String,
    /// The name of the model file.
    pub file_name: String,
    /// The amount of RAM required by the model (if known).
    pub ram_required: Option<i32>,
    /// Description of the model.
    pub description: String,
    /// URL to download the model.
    pub url: String,
    /// Prompt template for the completion model (if known and model type is for completions).
    pub prompt_template: Option<String>,
    /// System prompt for the model (if known and model type is for completions).
    pub system_prompt: Option<String>,
    /// Indicates whether the model is an embedding model.
    pub is_embedding_model: bool
}
