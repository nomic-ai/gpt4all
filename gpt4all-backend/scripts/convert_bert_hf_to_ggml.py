import sys
import struct
import json
import torch
import numpy as np

from transformers import AutoModel, AutoTokenizer

if len(sys.argv) < 3:
    print("Usage: convert-h5-to-ggml.py dir-model [use-f32]\n")
    print("  ftype == 0 -> float32")
    print("  ftype == 1 -> float16")
    sys.exit(1)

# output in the same directory as the model
dir_model = sys.argv[1]
fname_out = sys.argv[1] + "/ggml-model.bin"

with open(dir_model + "/tokenizer.json", "r", encoding="utf-8") as f:
    encoder = json.load(f)

with open(dir_model + "/config.json", "r", encoding="utf-8") as f:
    hparams = json.load(f)

with open(dir_model + "/vocab.txt", "r", encoding="utf-8") as f:
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
    fname_out = sys.argv[1] + "/ggml-model-" + ftype_str[ftype] + ".bin"


tokenizer = AutoTokenizer.from_pretrained(dir_model)
model = AutoModel.from_pretrained(dir_model, low_cpu_mem_usage=True)
print (model)

print(tokenizer.encode('I believe the meaning of life is'))

list_vars = model.state_dict()
for name in list_vars.keys():
    print(name, list_vars[name].shape, list_vars[name].dtype)

fout = open(fname_out, "wb")

print(hparams)

fout.write(struct.pack("i", 0x62657274)) # magic: ggml in hex
fout.write(struct.pack("i", hparams["vocab_size"]))
fout.write(struct.pack("i", hparams["max_position_embeddings"]))
fout.write(struct.pack("i", hparams["hidden_size"]))
fout.write(struct.pack("i", hparams["intermediate_size"]))
fout.write(struct.pack("i", hparams["num_attention_heads"]))
fout.write(struct.pack("i", hparams["num_hidden_layers"]))
fout.write(struct.pack("i", ftype))

for i in range(hparams["vocab_size"]):
    text = vocab[i][:-1] # strips newline at the end
    #print(f"{i}:{text}")
    data = bytes(text, 'utf-8')
    fout.write(struct.pack("i", len(data)))
    fout.write(data)

for name in list_vars.keys():
    data = list_vars[name].squeeze().numpy()
    if name in ['embeddings.position_ids', 'pooler.dense.weight', 'pooler.dense.bias']:
        continue
    print("Processing variable: " + name + " with shape: ", data.shape)

    n_dims = len(data.shape);

    # ftype == 0 -> float32, ftype == 1 -> float16
    if ftype == 1 and name[-7:] == ".weight" and n_dims == 2:
            print("  Converting to float16")
            data = data.astype(np.float16)
            l_type = 1
    else:
        l_type = 0

    # header
    str = name.encode('utf-8')
    fout.write(struct.pack("iii", n_dims, len(str), l_type))
    for i in range(n_dims):
        fout.write(struct.pack("i", data.shape[n_dims - 1 - i]))
    fout.write(str);

    # data
    data.tofile(fout)

fout.close()

print("Done. Output file: " + fname_out)
print("")
