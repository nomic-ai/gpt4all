use crate::model_source::file_utils::get_or_create_home_dir_sub_folder;
use crate::wrappers::loader::domain::{Device, ModelBuildVariant, ModelLoadOptions};
use crate::wrappers::loader::model_loader::ModelLoader;
use std::path::Path;

/// Sub-folder of home directory of your OS
const DEFAULT_MODELS_FOLDER: &str = ".cache/gpt4all";

impl Default for ModelLoader {
    fn default() -> Self {
        let out_dir = env!("OUT_DIR");

        Self {
            model_folder: get_or_create_home_dir_sub_folder(DEFAULT_MODELS_FOLDER),
            libraries_search_paths: vec![Path::new(out_dir).join("build")],
            model_load_options: Default::default(),
        }
    }
}

impl Default for ModelLoadOptions {
    fn default() -> Self {
        Self {
            nCtx: 2048,
            ngl: 100,
            device: Device::Cpu,
            threads: None,
            build_variant: ModelBuildVariant::Auto,
        }
    }
}
