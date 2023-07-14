# GPT4All Python Embedding API
GPT4All includes a super simple means of generating embeddings for your text documents.

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
The following table lists the generation speed for text documents of N tokens captured on an Intel i913900HX CPU with DDR5 5600 running with 8 threads under stable load.

| Tokens          | 2^7  | 2^9  | 2^11 | 2^13 | 2^14 |
| --------------- | ---- | ---- | ---- | ---- | ---- |
| Wall time (s)   | .02  | .08  | .24  | .96  | 1.9  |
| Tokens / Second | 6508 | 6431 | 8622 | 8509 | 8369 |


### API documentation
::: gpt4all.gpt4all.Embed4All
