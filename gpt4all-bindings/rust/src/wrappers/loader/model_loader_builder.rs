use std::path::PathBuf;

use crate::wrappers::loader::domain::ModelLoadOptions;
use crate::wrappers::loader::model_loader::ModelLoader;


/// Builder for creating instances of `ModelLoader`.
pub struct ModelLoaderBuilder {
    model_folder: PathBuf,
    libraries_search_paths: Vec<PathBuf>,
    model_load_options: ModelLoadOptions,
}

impl ModelLoaderBuilder {
    /// Creates a new `ModelLoaderBuilder` with predefined default values.
    pub fn new() -> Self {
        let default_loader = ModelLoader::default();

        ModelLoaderBuilder {
            model_folder: default_loader.model_folder,
            libraries_search_paths: default_loader.libraries_search_paths,
            model_load_options: default_loader.model_load_options,
        }
    }

    /// Sets the model folder path (folder, where models are located)
    pub fn model_folder(mut self, model_folder: &PathBuf) -> Self {
        self.model_folder = model_folder.clone();
        self
    }

    /// Adds a shared library search path.
    pub fn add_library_search_path(mut self, path: &PathBuf) -> Self {
        self.libraries_search_paths.push(path.clone());
        self
    }

    /// Sets the model load options.
    pub fn model_load_options(mut self, options: ModelLoadOptions) -> Self {
        self.model_load_options = options;
        self
    }

    /// Builds the `ModelLoader` instance.
    pub fn build(self) -> ModelLoader {
        ModelLoader {
            model_folder: self.model_folder,
            libraries_search_paths: self.libraries_search_paths,
            model_load_options: self.model_load_options,
        }
    }
}
