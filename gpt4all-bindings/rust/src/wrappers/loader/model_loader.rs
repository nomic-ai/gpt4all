use std::ffi::{c_char, CString};
use std::path::PathBuf;
use std::ptr::null_mut;
use std::slice;

use crate::bindings::{
    llmodel_available_gpu_devices, llmodel_gpu_init_gpu_device_by_string, llmodel_has_gpu_device,
    llmodel_isModelLoaded, llmodel_loadModel, llmodel_model, llmodel_model_create2,
    llmodel_required_mem, llmodel_setThreadCount, llmodel_set_implementation_search_path,
};
use crate::wrappers::completion::completion_model::CompletionModel;
use crate::wrappers::completion::domain::CompletionModelInfo;
use crate::wrappers::embedding::embedding_model::EmbeddingModel;
use crate::wrappers::ffi_utils::string_from_ptr;
use crate::wrappers::loader::domain::{CompletionModelConfig, Device, GpuDevice, ModelLoadOptions};
use crate::wrappers::loader::errors::ModelLoadingError;
use crate::wrappers::loader::errors::ModelLoadingError::BackendDeviceInitError;

/// A utility for loading and managing language models.
pub struct ModelLoader {
    /// The folder containing the language model files.
    pub(crate) model_folder: PathBuf,
    /// Paths to directories containing shared libraries.
    pub(crate) libraries_search_paths: Vec<PathBuf>,
    /// Options for loading the model (configuration)
    pub(crate) model_load_options: ModelLoadOptions,
}

impl ModelLoader {
    /// Loads a completion model.
    ///
    /// # Arguments
    ///
    /// * `model_file_name` - The filename of the model to load.
    /// * `config` - Configuration options for the completion model.
    ///
    /// # Returns
    ///
    /// A `CompletionModel` if the loading is successful, otherwise a `ModelLoadingError`.
    pub fn load_completion_model(
        &self,
        model_file_name: &str,
        config: CompletionModelConfig,
    ) -> Result<CompletionModel, ModelLoadingError> {
        let raw_model = self.get_raw_model(model_file_name)?;

        let completion_model = CompletionModel::new(
            raw_model,
            CompletionModelInfo {
                prompt_template: config.default_prompt_template.clone(),
            },
            self.model_load_options.nCtx,
        );

        Ok(completion_model)
    }

    /// Loads an embedding model.
    ///
    /// # Arguments
    ///
    /// * `model_file_name` - The filename of the model to load.
    ///
    /// # Returns
    ///
    /// An `EmbeddingModel` if the loading is successful, otherwise a `ModelLoadingError`.
    pub fn load_embedding_model(
        &self,
        model_file_name: &str,
    ) -> Result<EmbeddingModel, ModelLoadingError> {
        let raw_model = self.get_raw_model(model_file_name)?;

        let embedding_model = EmbeddingModel::new(raw_model);

        Ok(embedding_model)
    }

    /// Retrieves the initialized and configured (ready to be used) raw model from the specified file.
    fn get_raw_model(&self, model_file_name: &str) -> Result<llmodel_model, ModelLoadingError> {
        self.setup_libraries();

        let file_path = self.model_folder.join(model_file_name);
        let file_path = file_path.to_str().unwrap();
        let file_path = CString::new(file_path).expect("Failed to create CString");

        let model = self.create_raw_model(&file_path)?;

        self.load_raw_model(model, &file_path)?;

        self.init_device(model, &file_path)?;

        Ok(model)
    }

    /// Sets up library search paths for loading additional model libraries.
    fn setup_libraries(&self) {
        let libraries = self
            .libraries_search_paths
            .iter()
            .map(|path_buf| path_buf.to_str().unwrap().to_string())
            .collect::<Vec<String>>()
            .join(";");

        let search_path = CString::new(libraries).unwrap();

        unsafe {
            llmodel_set_implementation_search_path(search_path.as_ptr());
        }
    }

    /// Loads the raw model from the specified file path.
    fn load_raw_model(
        &self,
        raw_model: llmodel_model,
        file_path: &CString,
    ) -> Result<(), ModelLoadingError> {
        let loaded = unsafe {
            llmodel_loadModel(
                raw_model,
                file_path.as_ptr(),
                self.model_load_options.nCtx,
                self.model_load_options.ngl,
            )
        };

        if !loaded || unsafe { !llmodel_isModelLoaded(raw_model) } {
            return Err(ModelLoadingError::BackendLoadError);
        }

        Ok(())
    }

    /// Creates a raw model based on the file path.
    fn create_raw_model(&self, file_path: &CString) -> Result<llmodel_model, ModelLoadingError> {
        let build_variant_str =
            CString::new(self.model_load_options.build_variant.to_string()).unwrap();
        let load_model_error: *mut *const c_char = null_mut();

        let raw_model = unsafe {
            llmodel_model_create2(
                file_path.as_ptr(),
                build_variant_str.as_ptr(),
                load_model_error,
            )
        };

        if raw_model.is_null() {
            let error_msg = string_from_ptr(load_model_error as *const c_char)
                .unwrap_or("Unknown Error Message".to_string());

            return Err(ModelLoadingError::BackendCreateError(error_msg));
        }

        Ok(raw_model)
    }

    /// Initializes the device for model computation.
    fn init_device(
        &self,
        model: llmodel_model,
        model_path: &CString,
    ) -> Result<(), ModelLoadingError> {
        if self.model_load_options.device == Device::Cpu {
            return Ok(());
        } // CPU device is used by default

        let mem_required = unsafe {
            llmodel_required_mem(
                model,
                model_path.as_ptr(),
                self.model_load_options.nCtx,
                self.model_load_options.ngl,
            )
        };

        let device = CString::new(self.model_load_options.device.to_string()).unwrap();

        if unsafe { !llmodel_gpu_init_gpu_device_by_string(model, mem_required, device.as_ptr()) } {
            return Err(self.unavailable_gpu_error(mem_required));
        }

        if !unsafe { llmodel_has_gpu_device(model) } {
            return Err(BackendDeviceInitError(
                "Failed to initialize GPU device.".to_string(),
            ));
        }

        if let Some(thread_count) = self.model_load_options.threads {
            unsafe { llmodel_setThreadCount(model, thread_count) }
        }

        Ok(())
    }

    /// Creates 'ModelLoadingError' error with message about unavailable GPU devices.
    fn unavailable_gpu_error(&self, mem_required: usize) -> ModelLoadingError {
        let all_gpus = self.available_gpu_devices(0);

        let available_gpus = self.available_gpu_devices(mem_required);

        let unavailable_gpus: Vec<GpuDevice> = all_gpus
            .into_iter()
            .filter(|device| {
                !available_gpus
                    .iter()
                    .any(|available_device| device.index == available_device.index)
            })
            .collect();

        let mut error_msg = format!(
            "Unable to initialize model on GPU: '{}'.",
            self.model_load_options.device
        );
        error_msg += &format!("\nAvailable GPUs: {:?}.", available_gpus);
        error_msg += &format!(
            "\nUnavailable GPUs due to insufficient memory or features: {:?}.",
            unavailable_gpus
        );

        BackendDeviceInitError(error_msg)
    }

    /// Retrieves available GPU devices for model computation.
    fn available_gpu_devices(&self, mem_required: usize) -> Vec<GpuDevice> {
        let mut num_devices = 0;
        let devices_ptr = unsafe { llmodel_available_gpu_devices(mem_required, &mut num_devices) };

        if (!devices_ptr.is_null()) || (num_devices <= 0) {
            return vec![];
        }

        let gpus: Vec<GpuDevice> =
            unsafe { slice::from_raw_parts(devices_ptr, num_devices as usize) }
                .to_vec()
                .into_iter()
                .map(GpuDevice::from)
                .collect();

        gpus
    }
}
