# Converting From A Trained Huggingface Model to GGML Quantized Model

Currently, converting from a Huggingface Model to a GGML Quantized model is a tedious process that involves a few different steps. Here we will outline the current process.

`convert_llama_hf_to_ggml.py` is from [llama.cpp](https://github.com/ggerganov/llama.cpp/blob/master/convert.py) and doesn't rely on Huggingface or PyTorch.

The other scripts rely on Huggingface and PyTorch and are adapted from [ggml](https://github.com/ggerganov/ggml).

For the following example, we will use a LLaMa style model.

1. Install the depenedencies

    ```bash
    pip install -r requirements.txt
    ```

2. Convert the model to `ggml` format

```bash
python converter/convert_llama_hf_to_ggml.py <model_name> <output_dir> --outtype=<output_type>
```

1. Navigate to the `llama.cpp` directory

1. Build `llama.cpp`

    ```bash
    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    ```

1. Run the `quantize` binary

    ```bash
    ./quantize <ggmlfp32.bin>  <output_model.bin> <quantization_level>
    ```