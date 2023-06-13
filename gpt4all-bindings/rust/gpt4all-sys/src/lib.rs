#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[cfg(test)]
mod tests {
    use std::ffi::CString;
    use super::*;

    #[test]
    fn test_calling_into_lib() {
        let invalid_model_path = CString::new("invalid/model/path").unwrap();
        let ptr = unsafe {
            llmodel_model_create(invalid_model_path.as_ptr())
        };
        assert!(ptr.is_null());
    }

    #[test]
    fn test_calling_create() {
        let model_path = CString::new("/home/marcusdunn/IdeaProjects/gpt4all/gpt4all-bindings/rust/gpt4all/ggml-gpt4all-j-v1.3-groovy.bin").unwrap();
        let ptr = unsafe {
            llmodel_model_create(model_path.as_ptr())
        };
        assert!(!ptr.is_null());
    }
}