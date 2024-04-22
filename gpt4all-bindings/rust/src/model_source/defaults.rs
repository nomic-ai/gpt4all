use crate::model_source::file_utils::get_or_create_home_dir_sub_folder;
use crate::model_source::gpt4all_model_source::Gpt4AllModelSource;

/// Sub-folder of home directory of your OS
pub const DEFAULT_MODELS_FOLDER: &str = ".cache/gpt4all";


impl Default for Gpt4AllModelSource {
    fn default() -> Self {
        Self {
            url: "https://raw.githubusercontent.com/nomic-ai/gpt4all/main/gpt4all-chat/metadata/models3.json".to_string(),
            folder: get_or_create_home_dir_sub_folder(DEFAULT_MODELS_FOLDER)
        }
    }
}
