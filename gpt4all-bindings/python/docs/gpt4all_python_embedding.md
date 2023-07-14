# Embeddings
GPT4All supports generating high quality embeddings of arbitrary length documents of text using a CPU optimized contrastively trained [Sentence Transformer](https://www.sbert.net/). These embeddings are comparable in quality for many tasks with OpenAI.

## Quickstart

```bash
pip install gpt4all
```

### Generating embeddings
The embedding model will automatically be downloaded if not installed.

=== "Embed4All Example"
    ``` py
    from gpt4all import GPT4All, Embed4All
    text = 'The quick brown fox jumps over the lazy dog'
    embedder = Embed4All()
    output = embedder.embed(text)
    print(output)
    ```
=== "Output"
    ```
    [0.034696947783231735, -0.07192722707986832, 0.06923297047615051, ...]
    ```
### Speed of embedding generation
The following table lists the generation speed for text document captured on an Intel i913900HX CPU with DDR5 5600 running with 8 threads under stable load.

| Tokens          | 128  | 512  | 2048 | 8129 | 16,384 |
| --------------- | ---- | ---- | ---- | ---- | ---- |
| Wall time (s)   | .02  | .08  | .24  | .96  | 1.9  |
| Tokens / Second | 6508 | 6431 | 8622 | 8509 | 8369 |


### API documentation
::: gpt4all.gpt4all.Embed4All
