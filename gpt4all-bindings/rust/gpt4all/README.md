# gpt4all

This is a Rust binding for the Gpt4All library. It is a work in progress but is usable. It aims to be a very thin, safe
wrapper around the raw bindings, this goal is not complete - some calls require unsafe code as the gpt4all interface is
callback-heavy which makes it difficult to encapsulate in both an equally powerful and safe way. (suggestions welcome!)

## Usage

```rust
fn main() {
    let model = Model::new("ggml-gpt4all-j-v1.3-groovy.bin", "auto")
        .expect("Failed to load model")
        .initialize()
        .expect("failed to initialize model");

    let mut context = Context::default();

    model.prompt(
        "What is blue and smells like red paint?", // blue paint!
        prompt_callback,
        response_callback,
        recalculate_callback,
        &mut context,
    ).expect("Failed to prompt");
}
```