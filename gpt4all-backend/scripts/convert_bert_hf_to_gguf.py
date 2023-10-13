#!/usr/bin/env python3
from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

import gguf
import numpy as np
from transformers import AutoConfig, AutoModel, AutoTokenizer


if not 2 <= len(sys.argv) < 4:
    print("Usage: {} dir-model [ftype]\n".format(Path(__file__).name))
    print("  ftype == 0 -> float32")
    print("  ftype == 1 -> float16")
    sys.exit(1)

# output in the same directory as the model
dir_model = Path(sys.argv[1])

with open(dir_model / "vocab.txt", encoding="utf-8") as f:
    vocab = f.readlines()

# possible data types
#   ftype == 0 -> float32
#   ftype == 1 -> float16
#
# map from ftype to string
ftype_str = ["f32", "f16"]

ftype = 1
if len(sys.argv) > 2:
    ftype = int(sys.argv[2])
    if ftype < 0 or ftype > 1:
        print("Invalid ftype: " + str(ftype))
        sys.exit(1)

fname_out = dir_model / ("ggml-model-" + ftype_str[ftype] + ".gguf")


ARCH = gguf.MODEL_ARCH.BERT
gguf_writer = gguf.GGUFWriter(fname_out, gguf.MODEL_ARCH_NAMES[ARCH])

print("gguf: get model metadata")

config = AutoConfig.from_pretrained(dir_model)

block_count = config.num_hidden_layers
gguf_writer.add_name("BERT")
gguf_writer.add_context_length(config.max_position_embeddings)
gguf_writer.add_embedding_length(config.hidden_size)
gguf_writer.add_feed_forward_length(config.intermediate_size)
gguf_writer.add_block_count(block_count)
gguf_writer.add_head_count(config.num_attention_heads)
gguf_writer.add_file_type(ftype)

print("gguf: get tokenizer metadata")

try:
    with open(dir_model / "tokenizer.json", encoding="utf-8") as f:
        tokenizer_json = json.load(f)
except FileNotFoundError as e:
    print(f'Error: Missing {e.filename!r}', file=sys.stderr)
    sys.exit(1)

print("gguf: get wordpiece tokenizer vocab")

tokenizer = AutoTokenizer.from_pretrained(dir_model)
print(tokenizer.encode('I believe the meaning of life is'))

tokens: list[bytearray] = []
reverse_vocab = {id: encoded_tok for encoded_tok, id in tokenizer.vocab.items()}

# The number of tokens in tokenizer.json can differ from the expected vocab size.
# This causes downstream issues with mismatched tensor sizes when running the inference
for i in range(config.vocab_size):
    try:
        text = reverse_vocab[i]
    except KeyError:
        print(f"Key {i} not in tokenizer vocabulary. Padding with an arbitrary token.")
        pad_token = f"[PAD{i}]".encode("utf8")
        text = bytearray(pad_token)

    tokens.append(text)

gguf_writer.add_tokenizer_model("bert")  # wordpiece
gguf_writer.add_token_list(tokens)

special_vocab = gguf.SpecialVocab(dir_model, load_merges=True)
special_vocab.add_to_gguf(gguf_writer)

print("gguf: get tensor metadata")

model = AutoModel.from_pretrained(dir_model, config=config, low_cpu_mem_usage=True)
print(model)

tensor_map = gguf.get_tensor_name_map(ARCH, block_count)

list_vars = model.state_dict()
for name in list_vars.keys():
    print(name, list_vars[name].shape, list_vars[name].dtype)

for name in list_vars.keys():
    data = list_vars[name].squeeze().numpy()
    if name in ['embeddings.position_ids', 'pooler.dense.weight', 'pooler.dense.bias']:
        continue
    print("Processing variable:", name, "with shape:", data.shape)

    n_dims = len(data.shape)

    # ftype == 0 -> float32, ftype == 1 -> float16
    if ftype == 1 and name[-7:] == ".weight" and n_dims == 2:
        print("  Converting to float16")
        data = data.astype(np.float16)
        l_type = 1
    else:
        l_type = 0

    # map tensor names
    new_name = tensor_map.get_name(name, try_suffixes=(".weight", ".bias"))
    if new_name is None:
        print("Can not map tensor '" + name + "'")
        sys.exit()

    gguf_writer.add_tensor(new_name, data)


print("gguf: write header")
gguf_writer.write_header_to_file()
print("gguf: write metadata")
gguf_writer.write_kv_data_to_file()
print("gguf: write tensors")
gguf_writer.write_tensors_to_file()

gguf_writer.close()

print(f"gguf: model successfully exported to '{fname_out}'")
print()
