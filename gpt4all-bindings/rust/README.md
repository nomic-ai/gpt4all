# Rust GPT4All

This package contains a set of Rust bindings around the `llmodel` C-API.

Package on Crates: - **Crates.io**: [gpt4all](https://crates.io/crates/gpt4all)

Currently tested only on **MacOS**, **Linux (ubuntu)**

### Prerequisites

On Windows and Linux, building GPT4All requires the complete Vulkan SDK. You may download it from here: https://vulkan.lunarg.com/sdk/home

macOS users do not need Vulkan, as GPT4All will use Metal instead.

## Installation

The easiest way to install the Rust bindings for GPT4All is to use cargo:

```
cargo add gpt4all
```

This will download the latest version of the `gpt4all` package from Crates.

## Local Build

As an alternative to downloading via cargo, you may build the Rust bindings from source.

### Building the rust bindings

1. Clone GPT4All and change directory:
```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all.git
cd gpt4all/gpt4all-backend
```

2. Add the local GPT4All Rust crate to your project's Cargo.toml:
```
[dependencies]
gpt4all = { path = "..path_to_gpt4all../gpt4all/gpt4all-bindings/rust" }
```

## Usage

### Completions
Test it out! In a Rust script:

```rust
fn main() {
    // use default model loader
    let model_loader = ModelLoaderBuilder::new().build();

    // load completion model (should already be downloaded and located at default directory 'HOME_DIR/.cache/gpt4all)
    let model = model_loader
        .load_completion_model(NOUS_HERMES_MODEL_FILE, CompletionModelConfig {
            // configure default prompt template for loaded model
            default_prompt_template: Some("<|im_start|>user\n%1<|im_end|>\n<|im_start|>assistant\n%2<|im_end|>\n".to_string()),
        })
        .expect("Failed to load a model");

    // use stateless prompting use case
    let stateless_prompting_builder = StatelessPromptingBuilder::new(&model)
        .system_description("<|im_start|>system\nYou are helpful and kind math teacher.\n<|im_end|>\n")
        .add_reply_expectation("5 + 8, explain for me", "5 + 8 = 13 :)")
        .add_reply_expectation("10 + 8, explain for me", "10 + 8 = 18 :)")
        .add_reply_expectation("1 + 8, explain for me", "1 + 8 = 9 :)");
    
    let stateless_prompting = stateless_prompting_builder.build();

    // ask question
    let answer = stateless_prompting.ask("What is 5 + 5?\n");
    
    println!("{}", answer);
}
```


### Embeddings

```rust
fn main() {
    // use default model loader
    let model_loader = ModelLoaderBuilder::new().build();

    // load embedding model (should already be downloaded and located at default directory 'HOME_DIR/.cache/gpt4all)
    let embedding_model = model_loader
        .load_embedding_model("nomic-embed-text-v1.f16.gguf")
        .expect("Failed to load a model" );
    
    // configure embedding options
    let file_content = vec!["SOME FILE CONTENT".to_string()];
    let embedding_options = EmbeddingOptionsBuilder::new()
        .do_mean(true)
        .texts(&file_content)
        .build();

    // create embedding from configured options
    let embedding = embedding_model
        .create_embedding(embedding_options)
        .expect("failed to create embedding");
}
```

## Development

### How to generate bindings
```
# Install bindgen if you haven't already
cargo install bindgen

# Generate Rust bindings for llmodel_c.h
bindgen llmodel_c.h -o bindings.rs
```

### Publishing to crates
 - Scripts located at the './scripts' folder are used to prepare the project for publishing to Crates.io.
