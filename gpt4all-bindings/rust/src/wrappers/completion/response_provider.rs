use std::ffi::CStr;
use std::os::raw::c_char;
use std::ptr::addr_of_mut;
use crate::bindings::{llmodel_prompt_callback, llmodel_recalculate_callback, llmodel_response_callback};


// Struct to hold response data and callbacks for response handling
pub(crate) struct ResponseProvider {
    response_holder: String,
    response_part_callback: Option<fn(response_part: &str) -> bool>
}

// This static variable holds the response provider instance, providing context to the extern "C" function.
// It needs to be static to maintain compatibility with the extern "C" function signature,
// ensuring it can be used without changing the function signature.
// However, this design choice limits the ability to prompt multiple models asynchronously or in parallel.
// More details: https://doc.rust-lang.org/reference/items/static-items.html
static mut RESPONSE_PROVIDER: ResponseProvider = ResponseProvider {
    response_holder: String::new(),
    response_part_callback: None,
};


// Struct to hold callbacks for low-level C library model interactions
pub(crate) struct PromptCallbacks {
    pub prompt_callback: llmodel_prompt_callback,
    pub response_callback: llmodel_response_callback,
    pub recalculate_callback: llmodel_recalculate_callback
}

// Singleton implementation of the ResponseProvider
impl ResponseProvider {
    /// Creates a "new instance" of ResponseProvider (resets global static instance and returns it)
    pub fn new() -> &'static mut ResponseProvider {
        unsafe {
            // Clear existing data and callbacks
            RESPONSE_PROVIDER.response_holder.clear();
            RESPONSE_PROVIDER.response_part_callback = None;

            // Workaround for the upcoming 2024 Rust edition
            //  -- Disallow *references* to static mut
            //  -- https://github.com/rust-lang/rust/issues/114447
            let raw_pointer = addr_of_mut!(RESPONSE_PROVIDER);

            return &mut *raw_pointer
        }
    }

    /// Sets a callback function to be called for each response part (token) received.
    ///
    /// # Arguments
    ///
    /// * `callback` - A function pointer representing the callback function.
    ///                This function will be called for each response part received by the response provider.
    ///                It takes a single argument `response_part`, which is a reference to the received response part (token) as a string slice (`&str`).
    ///                The function should return a boolean indicating whether the processing should continue (`true`) or stop (`false`).
    pub fn subscribe_on_response_part(&mut self, callback: fn(response_part: &str) -> bool) {
        self.response_part_callback = Some(callback);
    }

    /// Extracts the accumulated response from the response holder and clears it
    pub fn extract_response(&mut self) -> String {
        let extracted_response = self.response_holder.clone();
        self.response_holder.clear();

        extracted_response
    }

    /// Retrieves the callbacks for model interactions.
    ///
    /// This function returns a `PromptCallbacks` struct containing extern "C" callback functions
    /// for handling prompt processing, response, and recalculation of context.
    ///
    /// # Returns
    ///
    /// A `PromptCallbacks` struct containing callback functions for model interactions.
    pub fn get_callbacks(&self) -> PromptCallbacks {
        PromptCallbacks {
            prompt_callback: Some(Self::prompt_callback),
            response_callback: Some(Self::response_callback),
            recalculate_callback: Some(Self::recalculate_callback)
        }
    }

    /// Callback function for prompt processing in the low-level C library model interactions.
    ///
    /// This function is intended to be used as a callback for processing prompts.
    ///
    /// # Arguments
    ///
    /// * `token_id` - The token id of the prompt.
    ///
    /// # Returns
    ///
    /// A boolean value indicating whether the model should keep processing.
    extern "C" fn prompt_callback(token_id: i32) -> bool { true }


    /// Callback function for response in the low-level C library model interactions.
    ///
    /// This function is intended to be used as a callback for processing responses.
    ///
    /// # Arguments
    ///
    /// * `token_id` - The token id of the response.
    /// * `response` - The response string. Note: a token id of -1 indicates the string is an error string.
    ///
    /// # Returns
    ///
    /// A boolean value indicating whether the model should keep generating.
    extern "C" fn response_callback(token_id: i32, response: *const c_char) -> bool {
        let response_part = unsafe { CStr::from_ptr(response) };
        if let Ok(response_str) = response_part.to_str() {
            unsafe { return RESPONSE_PROVIDER.add_response( response_str ) }
        }
        true
    }


    /// Adds a response part to the response holder and calls the response part callback if set.
    ///
    /// This function appends the provided response part (token) to the response holder string.
    /// If a response part callback is set, it is invoked with the response part as an argument.
    ///
    /// # Arguments
    ///
    /// * `response_part` - The response part (token) to add to the response holder.
    ///
    /// # Returns
    ///
    /// A boolean value indicating whether the processing should continue (`true`) or stop (`false`).
    fn add_response(&mut self, response_part: &str) -> bool {
        self.response_holder += response_part;
        self.response_part_callback.map_or(true, |callback| callback(response_part))
    }

    /// Callback function for recalculation of context in the low-level C library model interactions.
    ///
    /// This function is intended to be used as a callback for recalculation of context.
    ///
    /// # Arguments
    ///
    /// * `is_recalculating` - Indicates whether the model is recalculating the context.
    ///
    /// # Returns
    ///
    /// A boolean value indicating whether the model should keep generating.
    extern "C" fn recalculate_callback(is_recalculating: bool) -> bool { is_recalculating }
}
