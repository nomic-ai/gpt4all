# Convert Hugging Face fine-tuned bloom-like models to ggml format
#
# Usage:
#
#   python3 models/convert-h5-to-ggml.py 
#
# This script is similar to "convert-pt-to-ggml.py"
#

import io
import os
import sys
import struct
import json
import code
import torch
import numpy as np

from transformers import AutoTokenizer, AutoModelForCausalLM, AutoConfig, BloomForCausalLM

# ref: https://github.com/openai/gpt-2/blob/master/src/encoder.py
def bytes_to_unicode():
    """
    Returns list of utf-8 byte and a corresponding list of unicode strings.
    The reversible bpe codes work on unicode strings.
    This means you need a large # of unicode characters in your vocab if you want to avoid UNKs.
    When you're at something like a 10B token dataset you end up needing around 5K for decent coverage.
    This is a significant percentage of your normal, say, 32K bpe vocab.
    To avoid that, we want lookup tables between utf-8 bytes and unicode strings.
    And avoids mapping to whitespace/control characters the bpe code barfs on.
    """
    bs = list(range(ord("!"), ord("~")+1))+list(range(ord("¡"), ord("¬")+1))+list(range(ord("®"), ord("ÿ")+1))
    cs = bs[:]
    n = 0
    for b in range(2**8):
        if b not in bs:
            bs.append(b)
            cs.append(2**8+n)
            n += 1
    cs = [chr(n) for n in cs]
    return dict(zip(bs, cs))

if len(sys.argv) < 3:
    print("Usage: python convert-hf-to-ggml.py model_name dir-output [use-f32]")
    print("  model_name: name of the model to convert. Example: 'bigscience/bloomz-560m'")
    print("  dir-output: directory where the output file will be written")
    print("  use-f32:    if present, use float32 instead of float16")
    sys.exit(1)

model_name = sys.argv[1]
dir_out = sys.argv[2]

# make sure the output directory exists
os.makedirs(dir_out, exist_ok=True)

# possible data types
#   ftype == 0 -> float32
#   ftype == 1 -> float16
#
# map from ftype to string
ftype_str = ["f32", "f16"]
ftype = 1
if len(sys.argv) > 3:
    ftype = 0

tokenizer = AutoTokenizer.from_pretrained(model_name, trust_remote_code=True)
config = AutoConfig.from_pretrained(model_name, trust_remote_code=True)
hparams = config.to_dict()
print("Loading model: ", model_name)
model = AutoModelForCausalLM.from_pretrained(model_name, trust_remote_code=True, config=config, torch_dtype=torch.float16 if ftype == 1 else torch.float32, low_cpu_mem_usage=True)
print("Model loaded: ", model_name)


fname_out = dir_out + f"/ggml-model-{model_name.split('/')[-1]}-{ftype_str[ftype]}.bin"
fout = open(fname_out, "wb")

hparams["multiple_of"] = 1
fout.write(struct.pack("i", 0x67676d6c)) # magic: ggml in hex
fout.write(struct.pack("i", hparams["vocab_size"]))
# fout.write(struct.pack("i", hparams["seq_length"]))
fout.write(struct.pack("i", hparams["d_model"]))
fout.write(struct.pack("i", hparams["multiple_of"]))
fout.write(struct.pack("i", hparams["n_heads"]))
fout.write(struct.pack("i", hparams["n_layers"]))
fout.write(struct.pack("i", ftype))

# # Is this correct??
# dot_token = tokenizer.encode(".")[0]
# for i in range(hparams["vocab_size"]):
#     text = tokenizer.decode([i]).encode('utf-8')
#     fout.write(struct.pack("i", len(text)))
#     fout.write(text)
    
list_vars = model.state_dict()
for name in list_vars.keys():
    data = list_vars[name].squeeze().numpy()
    print("Processing variable: " + name + " with shape: ", data.shape)

    # we don't need these
    if name.endswith("attn.masked_bias") or name.endswith(".attn.bias"):
        print("  Skipping variable: " + name)
        continue

    if "Wqkv.weight" in name:
        # chunk qkv
        query, key, value = np.split(data, 3, axis=0)

        new_name = name.split("Wqkv.weight")[0]

        for (data, name) in [(query, new_name + "q_proj_w"), (key, new_name + "k_proj_w"), (value, new_name + "v_proj_w")]:
            print(f"Processing variable: {name} with shape: {data.shape}")
            n_dims = len(data.shape);

            # ftype == 0 -> float32, ftype == 1 -> float16
            ftype_cur = 0;
            if ftype != 0:
                if name[-7:] == ".weight" and n_dims == 2:
                    print("  Converting to float16")
                    data = data.astype(np.float16)
                    ftype_cur = 1
                else:
                    print("  Converting to float32")
                    data = data.astype(np.float32)
                    ftype_cur = 0
            else:
                if data.dtype != np.float32:
                    print("  Converting to float32")
                    data = data.astype(np.float32)
                    ftype_cur = 0

            # header
            str = name.encode('utf-8')
            fout.write(struct.pack("iii", n_dims, len(str), ftype_cur))
            for i in range(n_dims):
                fout.write(struct.pack("i", data.shape[n_dims - 1 - i]))
            fout.write(str);

            # data
            data.tofile(fout)
            fout.write(struct.pack("i", len(data)))

    else:

        n_dims = len(data.shape);

        # ftype == 0 -> float32, ftype == 1 -> float16
        ftype_cur = 0;
        if ftype != 0:
            if name[-7:] == ".weight" and n_dims == 2:
                print("  Converting to float16")
                data = data.astype(np.float16)
                ftype_cur = 1
            else:
                print("  Converting to float32")
                data = data.astype(np.float32)
                ftype_cur = 0
        else:
            if data.dtype != np.float32:
                print("  Converting to float32")
                data = data.astype(np.float32)
                ftype_cur = 0

        # header
        str = name.encode('utf-8')
        fout.write(struct.pack("iii", n_dims, len(str), ftype_cur))
        for i in range(n_dims):
            fout.write(struct.pack("i", data.shape[n_dims - 1 - i]))
        fout.write(str);

        # data
        data.tofile(fout)

fout.close()

print("Done. Output file: " + fname_out)
print("")