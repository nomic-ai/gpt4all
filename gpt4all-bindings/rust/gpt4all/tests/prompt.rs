use gpt4all::{Model};
use gpt4all::context::Context;

#[no_mangle]
extern "C" fn prompt_callback(_: i32) -> bool {
    true
}

#[no_mangle]
extern "C" fn response_callback(_: i32, response: *const std::os::raw::c_char) -> bool {
    let response = unsafe { std::ffi::CStr::from_ptr(response) };
    let response = response.to_str().unwrap();
    print!("{response}");
    true
}

#[no_mangle]
extern "C" fn recalculate_callback(is_recalculating: bool) -> bool {
    is_recalculating
}

#[test]
fn prompt() {
    let model = Model::new("ggml-gpt4all-j-v1.3-groovy.bin", "auto")
        .expect("Failed to load model");
    let model = model
        .initialize()
        .expect("failed to initialize model");
    let mut context = Context::default();
    model
        .prompt(
            "What is blue and smells like red paint?",
            prompt_callback,
            response_callback,
            recalculate_callback,
            &mut context,
        )
        .expect("Failed to prompt");
}
