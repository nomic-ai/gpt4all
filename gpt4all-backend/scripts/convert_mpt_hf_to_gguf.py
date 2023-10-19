#!/usr/bin/env python3
# Convert Hugging Face fine-tuned bloom-like models to ggml format
#
# Usage:
#
#   python3 models/convert-h5-to-ggml.py 
#
# This script is similar to "convert-pt-to-ggml.py"
#

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

import gguf
import numpy as np
import torch
from transformers import AutoTokenizer, AutoModelForCausalLM, AutoConfig, MptConfig
from transformers.models.gpt2 import tokenization_gpt2


if not 3 <= len(sys.argv) < 5:
    print("Usage: {} model-name dir-output [ftype]".format(Path(__file__).name))
    print("  model-name: name of the model to convert. Example: 'bigscience/bloomz-560m'")
    print("  dir-output: directory where the output file will be written")
    print("  ftype == 0 -> float32")
    print("  ftype == 1 -> float16")
    sys.exit(1)

dir_model = Path(sys.argv[1])
dir_out = Path(sys.argv[2])

# make sure the output directory exists
dir_out.mkdir(exist_ok=True)

# possible data types
#   ftype == 0 -> float32
#   ftype == 1 -> float16
#
# map from ftype to string
ftype_str = ["f32", "f16"]

ftype = 1
if len(sys.argv) > 3:
    ftype = int(sys.argv[3])
    if ftype < 0 or ftype > 1:
        print("Invalid ftype: " + str(ftype))
        sys.exit(1)

fname_out = dir_out / f"ggml-model-{dir_model.name}-{ftype_str[ftype]}.gguf"


ARCH = gguf.MODEL_ARCH.MPT
gguf_writer = gguf.GGUFWriter(fname_out, gguf.MODEL_ARCH_NAMES[ARCH])

print("gguf: get model metadata")

config = AutoConfig.from_pretrained(dir_model, trust_remote_code=True)

block_count = config.n_layers
gguf_writer.add_name("MPT")
gguf_writer.add_context_length(config.max_seq_len)
gguf_writer.add_embedding_length(config.d_model)
gguf_writer.add_block_count(block_count)
gguf_writer.add_feed_forward_length(4 * config.d_model)
gguf_writer.add_head_count(config.n_heads)
if kv_n_heads := config.attn_config.get('kv_n_heads'):
    gguf_writer.add_head_count_kv(kv_n_heads)
gguf_writer.add_max_alibi_bias(config.attn_config['alibi_bias_max'])
gguf_writer.add_layer_norm_eps(MptConfig().layer_norm_epsilon)  # use default from upstream transformers
gguf_writer.add_file_type(ftype)

clip_qkv = config.attn_config['clip_qkv']
if clip_qkv is not None:
    gguf_writer.add_clamp_kqv(clip_qkv)

print("gguf: get gpt2 tokenizer vocab")

tokenizer = AutoTokenizer.from_pretrained(dir_model)

special_ids = tokenizer.all_special_ids

reverse_vocab = {id: encoded_tok for encoded_tok, id in tokenizer.vocab.items()}
added_tokens = tokenizer.get_added_vocab().values()
byte_encoder = tokenization_gpt2.bytes_to_unicode()
byte_decoder = {v: k for k, v in byte_encoder.items()}

tokens: list[bytearray] = []
toktypes: list[gguf.TokenType] = []

# The number of tokens in tokenizer.json can differ from the expected vocab size.
# This causes downstream issues with mismatched tensor sizes when running the inference
for i in range(config.vocab_size):
    if i not in reverse_vocab:
        print(f"Key {i} not in tokenizer vocabulary. Padding with an arbitrary token.")
        pad_token = f"[PAD{i}]".encode("utf8")
        text = bytearray(pad_token)
    elif i in added_tokens:
        # these tokens are not encoded, for some reason
        text = bytearray(reverse_vocab[i].encode('utf-8'))
    else:
        text = bytearray([byte_decoder[c] for c in reverse_vocab[i]])

    tokens.append(text)

    # TODO(cebtenzzre): is there a better way to do this?
    toktypes.append(gguf.TokenType.CONTROL if i in special_ids else gguf.TokenType.NORMAL)

gguf_writer.add_tokenizer_model("gpt2")
gguf_writer.add_token_list(tokens)
gguf_writer.add_token_types(toktypes)

special_vocab = gguf.SpecialVocab(dir_model, load_merges=True)
special_vocab.add_to_gguf(gguf_writer)

print("gguf: get tensor metadata")

print("Loading model:", dir_model)
model = AutoModelForCausalLM.from_pretrained(
    dir_model, config=config, torch_dtype=torch.float16 if ftype == 1 else torch.float32,
    low_cpu_mem_usage=True, trust_remote_code=True,
)
print("Model loaded:", dir_model)

tensor_map = gguf.get_tensor_name_map(ARCH, block_count)

list_vars = model.state_dict()
for name in list_vars.keys():
    data = list_vars[name].squeeze().numpy()
    print("Processing variable:", name, "with shape:", data.shape)

    n_dims = len(data.shape)

    # ftype == 0 -> float32, ftype == 1 -> float16
    ftype_cur = 0
    # Keep token embeddings in fp32
    if ftype == 1 and name[-7:] == ".weight" and n_dims == 2 and ".wte" not in name:
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
