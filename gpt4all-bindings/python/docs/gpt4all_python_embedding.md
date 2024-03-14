# Embeddings
GPT4All supports generating high quality embeddings of arbitrary length text using any embedding model supported by llama.cpp. These include:
- [all-MiniLM-L6-v2](https://huggingface.co/sentence-transformers/all-MiniLM-L6-v2)
- [nomic-embed-text-v1](https://huggingface.co/nomic-ai/nomic-embed-text-v1)
- [nomic-embed-text-v1.5](https://huggingface.co/nomic-ai/nomic-embed-text-v1.5), with Matryoshka dimensionality reduction

## Quickstart
```bash
pip install gpt4all
```

### Generating embeddings
By default, embeddings will be generated on the CPU using all-MiniLM-L6-v2.

=== "Embed4All Example"
    ```py
    from gpt4all import Embed4All
    text = 'The quick brown fox jumps over the lazy dog'
    embedder = Embed4All()
    output = embedder.embed(text)
    print(output)
    ```
=== "Output"
    ```
    [0.034696947783231735, -0.07192722707986832, 0.06923297047615051, ...]
    ```

You can also use the GPU to accelerate the embedding model:

=== "GPU Example"
    ```py
    from gpt4all import Embed4All
    text = 'The quick brown fox jumps over the lazy dog'
    embedder = Embed4All(device='gpu')
    output = embedder.embed(text)
    print(output)
    ```
=== "Output"
    ```
    [0.034696947783231735, -0.07192722707986832, 0.06923297047615051, ...]
    ```

### Nomic Embed

Embed4All has built-in support for Nomic's open-source embedding model, [Nomic Embed]. When using this model, you must
specify the task type using the `prefix` argument. This may be one of `search_query`, `search_document`,
`classification`, or `clustering`. For retrieval applications, you should prepend `search_document` for all of your
documents and `search_query` for your queries.

[Nomic Embed]: https://blog.nomic.ai/posts/nomic-embed-text-v1

=== "Nomic Embed Example"
    ```py
    from gpt4all import Embed4All
    text = 'Who is Laurens van der Maaten?'
    embedder = Embed4All('nomic-embed-text-v1.f16.gguf')
    output = embedder.embed(text, prefix='search_query')
    print(output)
    ```
=== "Output"
    ```
    [-0.013357644900679588, 0.027070969343185425, -0.0232995692640543, ...]
    ```

### Truncation

Embed4All accepts a parameter called `long_text_mode`. This controls the behavior of Embed4All for texts longer than the
maximum sequence length of the embedding model.

In the default mode of "mean", Embed4All will break long inputs into chunks and average their embeddings to compute the
final result.

To change this behavior, you can set the `long_text_mode` parameter to "truncate", which will truncate the input to the
sequence length of the model before generating a single embedding.

=== "Truncation Example"
    ```py
    from gpt4all import Embed4All
    text = 'The ' * 512 + 'The quick brown fox jumps over the lazy dog'
    embedder = Embed4All()
    output = embedder.embed(text, long_text_mode="mean")
    print(output)
    print()
    output = embedder.embed(text, long_text_mode="truncate")
    print(output)
    ```
=== "Output"
    ```
    [0.0039850445464253426, 0.04558328539133072, 0.0035536508075892925, ...]

    [-0.009771130047738552, 0.034792833030223846, -0.013273917138576508, ...]
    ```


### Batching

You can send multiple texts to Embed4All in a single call. This can give faster results when individual texts are
siginficantly smaller than `n_ctx` tokens.

=== "Batching Example"
    ```py
    from gpt4all import Embed4All
    texts = ['The quick brown fox jumps over the lazy dog', 'Foo bar baz']
    embedder = Embed4All()
    output = embedder.embed(texts)
    print(output[0])
    print()
    print(output[1])
    ```
=== "Output"
    ```
    [0.03551332652568817, 0.06137588247656822, 0.05281158909201622, ...]

    [-0.03879690542817116, 0.00013223080895841122, 0.023148687556385994, ...]
    ```

The number of texts that can be embedded in one pass of the model is proportional to the `n_ctx` parameter of Embed4All.
You can try tweaking it to increase performance:
```py
embedder = Embed4All(n_ctx=1024)
```


### Dimensionality Reduction

The nomic-embed-text-v1.5 model supports Matryoshka dimensionality reduction. See the [blog post] for more info.

[blog post]: https://blog.nomic.ai/posts/nomic-embed-matryoshka

=== "Batching Example"
    ```py
    from gpt4all import Embed4All
    text = 'The quick brown fox jumps over the lazy dog'
    embedder = Embed4All('nomic-embed-text-v1.5.f16.gguf')
    output = embedder.embed(text, dimensionality=64)
    print(len(output))
    print(output)
    ```
=== "Output"
    ```
    64
    [-0.03567073494195938, 0.1301717758178711, -0.4333043396472931, ...]
    ```


### API documentation
::: gpt4all.gpt4all.Embed4All
