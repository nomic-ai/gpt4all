//! # gpt4all
//! Rust bindings for gpt4all
//!
//! ## Usage
//!
//! Add this to your `Cargo.toml`:
//!
//! ```toml
//! gpt4all = "0.1.0"
//! ```
//!
//! ## Examples
//!
//! ```no_run
//!
//!
//! use gpt4all::{Context, Model};
//!
//! #[no_mangle]
//! extern "C" fn prompt_callback(token_id: i32) -> bool {
//!     println!("token_id: {}", token_id);
//!     true
//! }
//!
//! #[no_mangle]
//! extern "C" fn response_callback(token_id: i32, response: *const std::os::raw::c_char) -> bool {
//!     let response = unsafe { std::ffi::CStr::from_ptr(response) };
//!     println!("token_id: {}, response: {}", token_id, response.to_str().expect("Failed to convert response to str"));
//!     true
//! }
//!
//! #[no_mangle]
//! extern "C" fn recalculate_callback(is_recalculating: bool) -> bool {
//!     println!("is_recalculating: {}", is_recalculating);
//!     is_recalculating
//! }
//!
//! #[test]
//! fn prompt() {
//!     let model = Model::new("ggml-gpt4all-j-v1.3-groovy.bin", "auto")
//!         .expect("Failed to load model")
//!         .initialize()
//!         .expect("failed to initialize model");
//!     let mut context = Context::default();
//!
//!     model.prompt(
//!         "What is blue and smells like red paint?",
//!         prompt_callback,
//!         response_callback,
//!         recalculate_callback,
//!         &mut context,
//!     ).expect("Failed to prompt");
//! }
//! ```

use gpt4all_sys::{
    llmodel_error, llmodel_loadModel, llmodel_model, llmodel_model_create2, llmodel_model_destroy,
    llmodel_prompt, llmodel_prompt_context,
};
use std::ffi::{c_int, CStr, CString};
use std::fmt::Debug;
use std::os::raw::c_char;
use std::ptr::{addr_of_mut, null_mut};
use std::str::Utf8Error;
use std::{slice};

/// A gpt4all model. This will have to be initialized via a call to [`Model::initialize`] to be
/// useful.
#[derive(Debug)]
pub struct Model {
    /// The underlying gpt4all model - This will be Some(model) unless the model has been
    /// initialized. in which case the model should have been dropped.
    model: Option<llmodel_model>,
    /// the path to the model file - used for initialization.
    model_path: CString,
}

impl Drop for Model {
    fn drop(&mut self) {
        if let Some(model) = self.model {
            unsafe {
                llmodel_model_destroy(model);
            }
        }
    }
}


/// An initialized model. Obtained by calling [`Model::initialize`] on a [`Model`].
pub struct InitModel {
    model: llmodel_model,
}

impl Drop for InitModel {
    fn drop(&mut self) {
        unsafe {
            llmodel_model_destroy(self.model);
        }
    }
}

/// An error returned by gpt4all when creating a model.
#[derive(Debug, thiserror::Error)]
#[error("Model error: {message} ({code})")]
pub struct ModelError {
    /// The error message returned by gpt4all.
    pub message: String,
    /// The error code returned by gpt4all.
    pub code: c_int,
}

#[derive(Debug, thiserror::Error)]
pub enum ModelErrorFromLLModelErrorError {
    /// The message returned by gpt4all was null.
    #[error("Null message")]
    NullMessage,
    /// The string returned by gpt4all was not valid utf8.
    #[error("{0}")]
    Utf8Error(#[from] Utf8Error),
}

impl TryFrom<llmodel_error> for ModelError {
    type Error = ModelErrorFromLLModelErrorError;

    fn try_from(value: llmodel_error) -> Result<Self, Self::Error> {
        if value.message.is_null() {
            return Err(ModelErrorFromLLModelErrorError::NullMessage);
        }
        // Safety:
        // - 1) message is not null and
        // - 2) we assume that gpt4all returns a valid null terminated string
        // - 3) this is never mutated on either side of the ffi boundary
        let message = unsafe { CStr::from_ptr(value.message) };
        let message = message.to_str()?.to_string();
        Ok(ModelError {
            message,
            code: value.code,
        })
    }
}

/// Errors that can occur when creating a model.
#[derive(Debug, thiserror::Error)]
pub enum CreateModelError {
    /// gpt4all returned an error when creating the model.
    #[error("{0}")]
    ModelError(#[from] ModelError),
    /// there was a null byte in the model path or build variant.
    #[error("{0}")]
    NullErr(#[from] std::ffi::NulError),
    /// there was an error converting a c string to a rust string.
    #[error("{0}")]
    Utf8Error(#[from] Utf8Error),
    /// there was an error creating the model - but no error was returned. This seems to occur when
    /// the model path is invalid, but is not guaranteed to be the only cause.
    #[error("Unknown error")]
    Unknown,
    #[error("{0}")]
    ModelErrorFromLLModelErrorError(#[from] ModelErrorFromLLModelErrorError),
}

/// Errors that can occur when prompting a model.
#[derive(Debug, thiserror::Error)]
pub enum PromptError {
    /// there was a null byte in the model path or build variant.
    #[error("Null error: {0}")]
    NullErr(#[from] std::ffi::NulError),
}

impl Model {
    /// Create a model instance. Recognises correct model type from file at model_path. This model
    /// will not be initialized.
    ///
    /// # Arguments
    ///
    /// * `model_path` - A string representing the path to the model file; will only be used to detect model type.
    /// * `build_variant` - A string representing the implementation to use (auto, default, avxonly, ...),
    ///
    /// # Examples
    ///
    /// ## Successfully create a model:
    ///
    /// ```no_run
    /// # use gpt4all::Model;
    /// let model = Model::new("ggml-gpt4all-j-v1.3-groovy.bin", "auto");
    /// assert!(model.is_ok());
    /// ```
    ///
    /// ## Invalid Model Path:
    ///
    /// ```
    /// # use gpt4all::Model;
    /// let model = Model::new("path/to/invalid/model", "auto");
    /// assert!(model.is_err());
    /// ```
    pub fn new(model_path: &str, build_variant: &str) -> Result<Model, CreateModelError> {
        let model_path_string = CString::new(model_path)?;
        let model_path = model_path_string.as_ptr();

        let build_variant = CString::new(build_variant)?;
        let build_variant = build_variant.as_ptr();

        let message = CString::new("No Message")?;

        let mut error = llmodel_error {
            code: 0xBEEF,
            message: message.as_ptr(),
        };

        let error = addr_of_mut!(error);

        // Safety: model_path and build_variant are not written to. and llmodel_model_create2 has
        // been at least a little checked.
        let model: llmodel_model =
            unsafe { llmodel_model_create2(model_path, build_variant, error) };

        // Safety: Model is either null or upholds the invariants of as_mut.
        let model = unsafe { model.as_mut() };

        // Safety: error is either null or upholds the invariants of as_ref.
        let Some(error) = (unsafe { error.as_ref() }) else {
            unreachable!("error is null")
        };

        let error = ModelError::try_from(*error)?;

        println!("model: {:?}", model);
        println!("error: {:?}", error);

        match (error, model) {
            (
                ModelError {
                    // either set to 0 by the app or set to 0xBEEF by us and never set.
                    code: 0 | 0xBEEF,
                    message: _,
                },
                Some(model),
            ) => {
                println!("model: {:?}", model);
                Ok(Model { model_path: model_path_string, model: Some(model) })
            }
            (
                err @ ModelError {
                    code: 0,
                    message: _,
                },
                None,
            ) => {
                unreachable!("model is null but error is {err}")
            }
            (model_error, None) => {
                println!("model_error: {:?}", model_error);
                Err(model_error.into())
            }
            (model_error, Some(_)) => {
                println!("model_error: {:?}", model_error);
                unreachable!("model is not null but error is not 0: {:?}", model_error)
            }
        }
    }

    /// Initialize the model. This is required before using the model.
    ///
    /// # Arguments
    ///
    /// * `self` - The model to initialize.
    ///
    /// # Examples
    ///
    /// ## Successfully initialize a model:
    ///
    /// ```no_run
    /// # use gpt4all::Model;
    /// # fn main() -> Result<(), Box<dyn std::error::Error>> {
    /// let model = Model::new("ggml-gpt4all-j-v1.3-groovy.bin", "auto")?;
    /// model.initialize()?;
    /// # Ok(())
    /// # }
    pub fn initialize(self) -> Result<InitModel, InitError> {
        if let Some(model) = self.model {
            let model_path = self.model_path.as_ptr();

            // Safety: FFI
            let load_model_success = unsafe { llmodel_loadModel(model, model_path) };

            if load_model_success {
                Ok(InitModel {
                    model,
                })
            } else {
                Err(InitError::Gpt4allError)
            }
        } else {
            unreachable!("The only place where model is set to None is in `initialize`, which consumes the `Model`")
        }
    }
}

/// An error that occurred while initializing a model.
#[derive(Debug, thiserror::Error)]
pub enum InitError {
    /// Gpt4All was unable to initialize the model. There is little information about why this may
    /// have occurred.
    #[error("Gpt4All Error")]
    Gpt4allError,
}

impl InitModel {
    /// Prompt and get the response back as a single string. This is the simplest way to use a model.
    /// The use of `extern "C" fn`s makes this API difficult to use. I imagine for most users
    /// there will only ever be a single model initialized at once in which case static global
    /// variables with appropriate synchronization could be used to make useful callbacks.
    ///
    /// # Arguments
    ///
    /// * `prompt` - A string representing the prompt to send to the model.
    /// * `context` - A mutable reference to a context to use for the prompt.
    /// * `prompt_callback` - A function that receives the most recent prompt token_id an returns a bool indicating whether to continue prompting.
    /// * `response_callback` - A function that receives the most recent response token_id and the response string and returns a bool indicating whether to continue prompting.
    /// * `recalculate_callback` - A function that receives a bool indicating whether the model is recalculating and returns a bool indicating whether to continue prompting.
    ///
    /// # Examples
    ///
    /// ```no_run
    ///use gpt4all::Model;
    ///use gpt4all::Context;
    ///
    /// #[no_mangle]
    /// extern "C" fn prompt_callback(token_id: i32) -> bool {
    ///     println!("token_id: {}", token_id);
    ///     true
    /// }
    ///
    /// #[no_mangle]
    /// extern "C" fn response_callback(token_id: i32, response: *const std::os::raw::c_char) -> bool {
    ///     let response = unsafe { std::ffi::CStr::from_ptr(response) };
    ///     println!("token_id: {}, response: {}", token_id, response.to_str().expect("Failed to convert response to str"));
    ///     true
    /// }
    ///
    /// #[no_mangle]
    /// extern "C" fn recalculate_callback(is_recalculating: bool) -> bool {
    ///     println!("is_recalculating: {}", is_recalculating);
    ///     is_recalculating
    /// }
    ///
    /// fn main() -> Result<(), Box<dyn std::error::Error>> {
    ///     let model = Model::new("ggml-gpt4all-j-v1.3-groovy.bin", "auto")?;
    ///     let model = model.initialize()?;
    ///     let mut context = Context::default();
    ///     model.prompt(
    ///         "What is blue and smells like red paint",
    ///         prompt_callback,
    ///         response_callback,
    ///         recalculate_callback,
    ///         &mut context,
    ///     )?;
    ///    Ok(())
    /// }
    pub fn prompt(
        &self,
        prompt: &str,
        prompt_callback: extern "C" fn(token_id: i32) -> bool,
        response_callback: extern "C" fn(token_id: i32, response: *const std::ffi::c_char) -> bool,
        recalculate_callback: extern "C" fn(is_recalculating: bool) -> bool,
        context: &mut Context,
    ) -> Result<(), PromptError> {
        let prompt = CString::new(prompt)?;
        let prompt = prompt.as_ptr();
        let context = addr_of_mut!(context.content);

        unsafe {
            llmodel_prompt(
                self.model,
                prompt,
                Some(prompt_callback),
                Some(response_callback),
                Some(recalculate_callback),
                context,
            )
        };

        Ok(())
    }
}

#[derive(Debug)]
pub struct Context {
    content: llmodel_prompt_context,
}

impl Context {
    pub fn logits(&self) -> &[f32] {
        // Safety: we assume that gpt4all has given us a valid pointer to contiguous memory of at
        // least logits_size elements
        unsafe { slice::from_raw_parts(self.content.logits, self.content.logits_size) }
    }
    pub fn tokens(&self) -> &[i32] {
        // Safety: we assume that gpt4all has given us a valid pointer to contiguous memory of at
        // least tokens_size elements
        unsafe { slice::from_raw_parts(self.content.tokens, self.content.tokens_size) }
    }
    pub fn n_past(&self) -> i32 {
        self.content.n_past
    }
    pub fn n_ctx(&self) -> i32 {
        self.content.n_ctx
    }
    pub fn n_predict(&self) -> i32 {
        self.content.n_predict
    }
    pub fn top_k(&self) -> i32 {
        self.content.top_k
    }
    pub fn top_p(&self) -> f32 {
        self.content.top_p
    }
    pub fn temp(&self) -> f32 {
        self.content.temp
    }
    pub fn n_batch(&self) -> i32 {
        self.content.n_batch
    }
    pub fn repeat_penalty(&self) -> f32 {
        self.content.repeat_penalty
    }
    pub fn repeat_last_n(&self) -> i32 {
        self.content.repeat_last_n
    }
    pub fn context_erase(&self) -> f32 {
        self.content.context_erase
    }
}

impl Default for Context {
    fn default() -> Self {
        Self {
            content: llmodel_prompt_context {
                // The python bindings seem to do this? (not super familiar with python ffi)
                logits: null_mut(),
                // The python bindings seem to do this? (not super familiar with python ffi)
                tokens: null_mut(),
                logits_size: 0,
                tokens_size: 0,
                n_past: 0,
                n_ctx: 1024,
                n_predict: 128,
                top_k: 40,
                top_p: 0.9,
                temp: 0.1,
                n_batch: 8,
                repeat_penalty: 1.2,
                repeat_last_n: 10,
                context_erase: 0.5,
            },
        }
    }
}