#!/usr/bin/env python3
from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

import gguf
import numpy as np
from sentencepiece import SentencePieceProcessor
from transformers import AutoConfig, AutoModelForCausalLM, AutoTokenizer


if not 2 <= len(sys.argv) < 4:
    print("Usage: {} dir-model [ftype]\n".format(Path(__file__).name))
    print("  ftype == 0 -> float32")
    print("  ftype == 1 -> float16")
    sys.exit(1)

# output in the same directory as the model
dir_model = Path(sys.argv[1])

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

fname_out = dir_model / ("ggml-replit-code-v1-3b-" + ftype_str[ftype] + ".gguf")


ARCH = gguf.MODEL_ARCH.MPT
gguf_writer = gguf.GGUFWriter(fname_out, gguf.MODEL_ARCH_NAMES[ARCH])

print("gguf: get model metadata")

config = AutoConfig.from_pretrained(dir_model)

block_count = config.n_layers
gguf_writer.add_name("Replit")
gguf_writer.add_context_length(config.max_seq_len)
gguf_writer.add_embedding_length(config.d_model)
gguf_writer.add_block_count(block_count)
gguf_writer.add_feed_forward_length(4 * config.d_model)
gguf_writer.add_head_count(config.n_heads)
gguf_writer.add_max_alibi_bias(config.attn_config.alibi_bias_max)
gguf_writer.add_layer_norm_eps(config.layer_norm_epsilon)
gguf_writer.add_file_type(ftype)

clip_qkv = config.attn_config.clip_qkv
if clip_qkv is not None:
    gguf_writer.add_clamp_kqv(clip_qkv)

print("gguf: get sentencepiece tokenizer vocab")

tokenizer = SentencePieceProcessor(str(dir_model / "spiece.model"))
#print(tokenizer.encode('I believe the meaning of life is'))

tokens: list[bytearray] = []
scores: list[float] = []
toktypes: list[int] = []

for i in range(tokenizer.vocab_size()):
    tokens.append(tokenizer.id_to_piece(i).encode('utf-8'))
    scores.append(tokenizer.get_score(i))

    toktype = gguf.TokenType.NORMAL
    if tokenizer.is_unknown(i):
        toktype = gguf.TokenType.UNKNOWN
    elif tokenizer.is_control(i):
        toktype = gguf.TokenType.CONTROL
    elif tokenizer.is_unused(i):
        toktype = gguf.TokenType.UNUSED
    elif tokenizer.is_byte(i):
        toktype = gguf.TokenType.BYTE

    toktypes.append(toktype)

gguf_writer.add_tokenizer_model("llama")  # sentencepiece
gguf_writer.add_token_list(tokens)
gguf_writer.add_token_scores(scores)
gguf_writer.add_token_types(toktypes)

special_vocab = gguf.SpecialVocab(dir_model, load_merges=True)
special_vocab.add_to_gguf(gguf_writer)

print("gguf: get tensor metadata")

model = AutoModelForCausalLM.from_pretrained(dir_model, config=config, low_cpu_mem_usage=True)
#print(model)

tensor_map = gguf.get_tensor_name_map(ARCH, block_count)

list_vars = model.state_dict()
for name in list_vars.keys():
    print(name, list_vars[name].shape, list_vars[name].dtype)

print(config)

for name in list_vars.keys():
    data = list_vars[name].squeeze().numpy()
    print("Processing variable:", name, "with shape:", data.shape)

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
