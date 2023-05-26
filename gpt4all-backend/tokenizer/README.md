# BPE tokenizer

This is a C++ implementation of the encoding/decoding functions of a pretrained GPT-2 style BPE tokenizer. It is meant to be compatible with the GPT-J and MPT-7B tokenizers that were trained with HuggingFace [`tokenizers`](https://github.com/huggingface/tokenizers), and only implements the necessary functionality for those models (it is assumed that strings should always be [normalized](https://en.wikipedia.org/wiki/Unicode_equivalence) to Unicode NFC form and split with the GPT-2 "pretokenizing" [regular expression](https://github.com/karpathy/minGPT/blob/37baab71b9abea1b76ab957409a1cc2fbfba8a26/mingpt/bpe.py#L92))

## Converting a tokenizer file

`scripts/gen_tokenizer_include.py` can be used to convert a huggingface `tokenizers` `tokenizer.json` file into a C++ header file:

```bash
# get tokenizer.json
cd gpt4all-backend
wget -O /tmp/gptj-tokenizer.json https://huggingface.co/nomic-ai/gpt4all-j/raw/main/tokenizer.json
python ./scripts/gen_tokenizer_include.py /tmp/gptj-tokenizer.json gptj > ./tokenizer/gptj_tokenizer_config.h
```
