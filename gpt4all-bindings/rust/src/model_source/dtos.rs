use crate::model_source::domain::ModelInfo;
use serde::Deserialize;

/// Data transfer object for deserializing model information.
#[derive(Deserialize)]
pub struct ModelInfoDto {
    /// MD5 checksum of the model file.
    pub md5sum: String,
    /// The name of the model.
    pub name: String,
    /// The name of the model file.
    #[serde(rename = "filename")]
    pub file_name: String,
    /// The size of the model file.
    #[serde(rename = "filesize")]
    pub file_size: String,
    /// The amount of RAM required by the model (if known).
    #[serde(rename = "ramrequired")]
    pub ram_required: String,
    /// Description of the model.
    pub description: String,
    /// URL to download the model.
    pub url: String,
    /// Optional prompt template for the model.
    #[serde(rename = "promptTemplate")]
    pub prompt_template: Option<String>,
    /// Optional system prompt for the model.
    #[serde(rename = "systemPrompt")]
    pub system_prompt: Option<String>,
    /// Indicates whether the model is an embedding model.
    #[serde(rename = "embeddingModel")]
    pub embedding_model: Option<bool>,
}

impl From<ModelInfoDto> for ModelInfo {
    fn from(model_info_dto: ModelInfoDto) -> Self {
        Self {
            name: model_info_dto.name,
            url: model_info_dto.url,
            file_name: model_info_dto.file_name,
            ram_required: model_info_dto.ram_required.parse::<i32>().ok(),
            description: model_info_dto.description,
            system_prompt: model_info_dto.system_prompt,
            prompt_template: model_info_dto.prompt_template,
            is_embedding_model: model_info_dto.embedding_model.unwrap_or(false),
        }
    }
}
