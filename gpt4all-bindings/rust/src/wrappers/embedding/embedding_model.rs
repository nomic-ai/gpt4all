use std::ffi::{c_char, CString};
use std::ptr::{null, null_mut};
use std::slice;

use crate::bindings::{llmodel_embed, llmodel_free_embedding, llmodel_model, llmodel_model_destroy};
use crate::wrappers::embedding::domain::{Embedding, EmbeddingOptions};
use crate::wrappers::embedding::errors::EmbeddingError;
use crate::wrappers::ffi_utils::string_from_ptr;


/// Wrapper for handling embedding operations.
pub struct EmbeddingModel {
    /// The raw model pointer.
    raw_model: llmodel_model
}


impl EmbeddingModel {
    /// Creates a new `EmbeddingModel` instance.
    pub fn new(raw_model: llmodel_model) -> Self {
        Self {  raw_model }
    }

    /// Creates an embedding for the given options.
    ///
    /// # Arguments
    ///
    /// * `embedding_options` - Options for creating the embedding.
    ///
    /// # Returns
    ///
    /// * `Ok(Embedding)` if the embedding creation is successful.
    /// * `Err(EmbeddingError)` if an error occurs during the process.
    pub fn create_embedding(&self, embedding_options: EmbeddingOptions) -> Result<Embedding, EmbeddingError> {
        // Collect texts into CString objects
        let texts: Vec<CString> = embedding_options.texts
            .iter()
            .map(|text| CString::new(text.as_str()).expect("Failed to convert text to CString"))
            .collect();

        // Create a vector to hold the raw pointers to the C-style strings
        let mut texts_ptrs: Vec<*const c_char> = texts.iter().map(|text| text.as_ptr()).collect();

        // Append a null pointer to signify the end of the array
        texts_ptrs.push(null());

        // Prepare prefix
        let prefix = embedding_options.prefix.map(|val| { CString::new(val).unwrap()});
        let prefix_addr = prefix.map_or(null(), |val| { val.as_ptr() });

        // OUT PARAMETERS (returns)
        let mut embedding_size: usize = 0;
        let mut token_count: usize = 0; // Return location for the number of prompt tokens processed, or NULL.
        let mut error_msg_ptr: *const c_char = null_mut();

        // TODO: Add Cancellation CB to EmbeddingOptions (Updated Bindings), currently set to None (<==> Null)
        let embedding_ptr = unsafe {
            llmodel_embed(self.raw_model,
                          texts_ptrs.as_mut_ptr(),
                          &mut embedding_size,
                          prefix_addr,
                          embedding_options.dimensionality,
                          &mut token_count,
                          true,
                          embedding_options.atlas,
                          None,
                          &mut error_msg_ptr)
        };


        // Handle errors
        if embedding_ptr.is_null() {
            return self.handle_embedding_error(error_msg_ptr);
        }

        // Extract vectors
        let vectors = self.extract_vectors(embedding_ptr, embedding_size, embedding_options.texts.len());

        // Return embedding
        Ok(Embedding::new(vectors))
    }

    /// Extracts vectors from the embedding output.
    ///
    /// # Arguments
    ///
    /// * `embedding_ptr` - Pointer to the embedding output.
    /// * `embedding_size` - Size of the embedding output.
    /// * `embedded_texts_count` - Number of texts embedded.
    ///
    /// # Returns
    ///
    /// A vector of vectors (`Vec<Vec<f32>>`) containing the extracted vectors.
    fn extract_vectors(&self,
                      embedding_ptr: *mut f32,
                      embedding_size: usize,
                      embedded_texts_count: usize
    ) -> Vec<Vec<f32>> {
        // Convert the raw pointer with embedding data to a Rust Vec<f32>
        let embedding_output: Vec<f32> = unsafe {
            slice::from_raw_parts(embedding_ptr, embedding_size)
        }.to_vec();

        // Calculate the size of each vector (embedding)
        let vector_size = embedding_size / embedded_texts_count;
        let mut vectors = Vec::new();

        // Iterate over the embedding output and extract vectors
        for i in (0..embedding_size).step_by(vector_size) {
            let end_index = (i + vector_size).min(embedding_size);
            let vector_data = &embedding_output[i..end_index];
            vectors.push(vector_data.to_vec());
        }

        // Free the memory allocated for the embedding output
        unsafe { llmodel_free_embedding(embedding_ptr); }

        // Return the extracted vectors
        vectors
    }

    /// Handles embedding errors.
    ///
    /// # Arguments
    ///
    /// * `error_msg_ptr` - Raw Pointer to the error message string, if any.
    ///
    /// # Returns
    ///
    /// * `Ok(Embedding)` if the embedding creation is successful.
    /// * `Err(EmbeddingError)` if an error occurs during the process.
    fn handle_embedding_error(&self, error_msg_ptr: *const c_char) -> Result<Embedding, EmbeddingError> {
        let error_msg = string_from_ptr(error_msg_ptr as *const c_char)
            .unwrap_or("Unknown Error".to_string());

        Err(EmbeddingError::BackendEmbedFailed(error_msg))
    }

    /// Disposes of the `EmbeddingModel` instance.
    pub fn dispose(self) {
        unsafe { llmodel_model_destroy(self.raw_model); }
    }
}
