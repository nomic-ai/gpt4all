use std::ffi::{c_char, CStr};

/// Converts a null-terminated C string pointer to a Rust String.
///
/// This function converts a null-terminated C string pointer to a Rust String if the pointer
/// is not null. If the pointer is null or the conversion fails, it returns None.
///
/// # Arguments
///
/// * `ptr` - The null-terminated C string pointer to be converted.
///
/// # Returns
///
/// An Option containing the converted Rust String if successful, or None otherwise.
pub fn string_from_ptr(ptr: *const c_char) -> Option<String> {
    if ptr.is_null() {
        return None;
    }

    return unsafe {
        match CStr::from_ptr(ptr).to_str() {
            Ok(s) => Some(s.to_string()),
            Err(_) => None,
        }
    };
}
