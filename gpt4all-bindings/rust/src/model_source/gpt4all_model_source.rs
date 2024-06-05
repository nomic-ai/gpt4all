use std::fs;
use std::path::PathBuf;

use crate::model_source::domain::{ModelInfo, ModelSource};
use crate::model_source::dtos::ModelInfoDto;
use crate::model_source::errors::ModelSourceError;
use crate::model_source::errors::ModelSourceError::ModelDownloadError;
use crate::model_source::file_utils::download_file_with_integrity;

/// A model source implementation for fetching models from the GPT-4-All repository.
pub struct Gpt4AllModelSource {
    /// The URL of the GPT-4-All repository.
    pub url: String,
    /// The local folder where models will be stored after downloading.
    pub folder: PathBuf
}

impl Gpt4AllModelSource {
    /// Internal method to fetch available model information DTOs from the repository.
    async fn _fetch_available_models(&self) -> Result<Vec<ModelInfoDto>, ModelSourceError> {
        let response = reqwest::get(&self.url).await.map_err(|_| ModelSourceError::FetchError)?;

        let model_info_dto: Vec<ModelInfoDto> = response
            .json().await
            .map_err(|_| ModelSourceError::ModelInfoParsingError)?;

        Ok(model_info_dto)
    }
}

impl ModelSource for Gpt4AllModelSource {
    /// Fetches available models from the GPT-4-All repository.
    async fn fetch_all_models(&self) -> Result<Vec<ModelInfo>, ModelSourceError> {
        let model_info_dto = self._fetch_available_models().await?;

        let model_info = model_info_dto
            .into_iter()
            .map(ModelInfo::from)
            .collect();

        Ok(model_info)
    }

    /// Downloads a specific model from the GPT-4-All repository, if it does not already exist in 'folder'
    async fn download(&self, model_file_name: &str) -> Result<String, ModelSourceError> {
        let available_models = self._fetch_available_models().await?;

        let model_info = available_models
            .iter().find(|&model_info| { model_info.file_name == model_file_name })
            .ok_or(ModelSourceError::ModelNotFoundError)?;

        let path =  self.folder.join(&model_info.file_name);

        let path_str = path
            .to_str()
            .ok_or(ModelDownloadError)?
            .to_string();

        // If model is already downloaded, just return the path
        if path.exists() { return Ok(path_str) }

        download_file_with_integrity(&model_info.url, &path_str, &model_info.md5sum)
            .await.map_err(|_| { ModelDownloadError })?;

        Ok(path_str)
    }

    /// Retrieves the names of local models from provided folder (already downloaded from GPT-4-All repository).
    async fn get_local_models(&self) -> Result<Vec<String>, ModelSourceError> {
        let mut local_models = Vec::new();

        // Iterate over all entries in the folder
        if let Ok(entries) = fs::read_dir(&self.folder) {
            for entry in entries {
                if let Ok(entry) = entry {
                    // Check if the entry is a file and has the extension .gguf
                    if let Some(file_name) = entry.file_name().to_str() {
                        if entry.file_type().map(|ft| ft.is_file()).unwrap_or(false)
                            && file_name.ends_with(".gguf") {
                            local_models.push(file_name.to_string());
                        }
                    }
                }
            }
        } else {
            return Err(ModelSourceError::FetchError);
        }

        Ok(local_models)
    }
}
