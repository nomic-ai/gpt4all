//! Modify the global llmodel implementation search path for the GPT4All library.
//! To avoid concurrent modification of the search path, you must obtain the lock using
//! [`SearchPath:get_static`].
//!
//! # Example
//!
//! ## Get the default search path
//!
//! ```
//!# use gpt4all::search_path::SearchPath;
//!
//!# fn main() -> Result<(), Box<dyn std::error::Error>> {
//! let search_path = SearchPath::get_static().read().expect("failed to get search path");
//! assert_eq!(search_path.get(), ".");
//!#     Ok(())
//!# }
//! ```
//! ## Set the search path
//!
//! ```
//!# use gpt4all::search_path::SearchPath;
//!
//!# fn main() -> Result<(), Box<dyn std::error::Error>> {
//! let mut search_path = SearchPath::get_static().write().expect("failed to get search path");
//! search_path.set("path/to/implementation")?;
//! assert_eq!(search_path.get(), "path/to/implementation");
//!#     Ok(())
//!# }
//! ```

use gpt4all_sys::{llmodel_get_implementation_search_path, llmodel_set_implementation_search_path};
use std::ffi::{CStr, CString, NulError};
use std::sync::RwLock;

/// A unit struct used to represent access to the global search path.
/// This struct is not constructable, and can only be obtained by calling `SearchPath::get()` which
/// will return the global lock.
pub struct SearchPath {
    /// the underlying rust memory for the search path, set to none if the search path has not been
    /// set yet.
    string: Option<CString>,
}

static SEARCH_PATH: RwLock<SearchPath> = RwLock::new(SearchPath { string: None });

impl SearchPath {
    /// Get the global search path lock.
    ///
    /// # Example
    ///
    /// ```
    /// use gpt4all::search_path::SearchPath;
    /// let lock = SearchPath::get_static();
    /// ```
    pub fn get_static() -> &'static RwLock<SearchPath> {
        &SEARCH_PATH
    }

    /// Get the global search path.
    ///
    /// # Example
    ///
    /// ```
    /// use gpt4all::search_path::SearchPath;
    /// let lock = SearchPath::get_static();
    /// assert_eq!(lock.read().unwrap().get(), ".");
    /// ```
    pub fn get(&self) -> &str {
        let str = unsafe { llmodel_get_implementation_search_path() };

        // the string should be the same as the one we have stored in rust memory or None
        debug_assert!(
            self.string
                .as_ref()
                .map_or_else(|| true, |s| s.as_c_str() == unsafe { CStr::from_ptr(str) }),
            "search path is not the same as the one stored in SearchPath (has it been modified by something else?)"
        );

        let cstr = unsafe { CStr::from_ptr(str) };

        cstr.to_str().expect("invalid search path")
    }

    /// Set the global search path.
    /// This function will return an error if [`search_path`] contains a null byte.
    ///
    /// # Example
    ///
    /// ```
    /// use gpt4all::search_path::SearchPath;
    /// let mut lock = SearchPath::get_static();
    /// lock.write().unwrap().set("path/to/implementation").unwrap();
    /// assert_eq!(lock.read().unwrap().get(), "path/to/implementation");
    /// ```
    pub fn set(&mut self, search_path: &str) -> Result<(), NulError> {
        let c_string = CString::new(search_path)?;

        unsafe {
            llmodel_set_implementation_search_path(c_string.as_ptr());
        }

        self.string = Some(c_string);

        Ok(())
    }
}
