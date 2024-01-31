#!/usr/bin/env python3
# Convert GPT-J-6B h5 transformer model to ggml format
#
# Load the model using GPTJForCausalLM.
# Iterate over all variables and write them to a binary file.
#
# For each variable, write the following:
#   - Number of dimensions (int)
#   - Name length (int)
#   - Dimensions (int[n_dims])
#   - Name (char[name_length])
#   - Data (float[n_dims])
#
# By default, the bigger matrices are converted to 16-bit floats.
# This can be disabled by adding the "ftype" CLI argument.
#
# At the start of the ggml file we write the model parameters
# and vocabulary.
#

from __future__ import annotations

import sys
import struct
import json
from pathlib import Path

import gguf
import numpy as np
from transformers import AutoConfig, AutoTokenizer, GPTJForCausalLM
from transformers.models.gpt2 import tokenization_gpt2


if not 2 <= len(sys.argv) < 4:
    print("Usage: python {} dir-model [ftype]\n".format(Path(__file__).name))
    print("  ftype == 0 -> float32")
    print("  ftype == 1 -> float16")
    sys.exit(1)

# output in the same directory as the model
dir_model = Path(sys.argv[1])
fname_out = dir_model / "ggml-model.gguf"

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


ARCH = gguf.MODEL_ARCH.GPTJ
gguf_writer = gguf.GGUFWriter(fname_out, gguf.MODEL_ARCH_NAMES[ARCH])

print("gguf: get model metadata")

config = AutoConfig.from_pretrained(dir_model)

block_count = config.n_layer
gguf_writer.add_name("GPT-J")
gguf_writer.add_context_length(config.n_positions)
gguf_writer.add_embedding_length(config.n_embd)
gguf_writer.add_block_count(block_count)
gguf_writer.add_feed_forward_length(4 * config.n_embd)
gguf_writer.add_head_count(config.n_head)
gguf_writer.add_rope_dimension_count(config.rotary_dim)
gguf_writer.add_layer_norm_eps(config.layer_norm_epsilon)
gguf_writer.add_file_type(ftype)

print("gguf: get gpt2 tokenizer vocab")

tokenizer = AutoTokenizer.from_pretrained(dir_model)

reverse_vocab = {id: encoded_tok for encoded_tok, id in tokenizer.vocab.items()}
byte_encoder = tokenization_gpt2.bytes_to_unicode()
byte_decoder = {v: k for k, v in byte_encoder.items()}

tokens: list[bytearray] = []

for i in range(config.vocab_size):
    if i in reverse_vocab:
        try:
            text = bytearray([byte_decoder[c] for c in reverse_vocab[i]])
        except KeyError:
            text = bytearray()
            for c in reverse_vocab[i]:
                if ord(c) < 256:  # single byte character
                    text.append(byte_decoder[c])
                else:  # multibyte special token character
                    text.extend(c.encode('utf-8'))
    else:
        print(f"Key {i} not in tokenizer vocabulary. Padding with an arbitrary token.")
        pad_token = f"[PAD{i}]".encode("utf8")
        text = bytearray(pad_token)

    tokens.append(text)


gguf_writer.add_tokenizer_model("gpt2")
gguf_writer.add_token_list(tokens)

special_vocab = gguf.SpecialVocab(dir_model, load_merges=True)
special_vocab.add_to_gguf(gguf_writer)

print("gguf: get tensor metadata")

model = GPTJForCausalLM.from_pretrained(dir_model, config=config, low_cpu_mem_usage=True)
#print (model)

tensor_map = gguf.get_tensor_name_map(ARCH, block_count)

list_vars = model.state_dict()
#print (list_vars)

for name in list_vars.keys():
    data = list_vars[name].squeeze().numpy()
    print("Processing variable:", name, "with shape:", data.shape)

    # we don't need these
    if name.endswith("attn.masked_bias") or name.endswith(".attn.bias"):
        print("  Skipping variable:", name)
        continue

    n_dims = len(data.shape)

    # ftype == 0 -> float32, ftype == 1 -> float16
    ftype_cur = 0
    if ftype == 1 and name[-7:] == ".weight" and n_dims == 2:
        print("  Converting to float16")
        data = data.astype(np.float16)
        ftype_cur = 1
    elif ftype == 1 or data.dtype != np.float32:
        print("  Converting to float32")
        data = data.astype(np.float32)
        ftype_cur = 0

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
