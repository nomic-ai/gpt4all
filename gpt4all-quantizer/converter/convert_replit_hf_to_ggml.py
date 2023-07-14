import os
from pathlib import Path
import sys
import struct
import json
import numpy as np
from transformers import AutoModelForCausalLM, AutoTokenizer
from huggingface_hub import hf_hub_download
import sentencepiece.sentencepiece_model_pb2 as model

if len(sys.argv) < 3:
    print("Usage: convert-h5-to-ggml.py model_name out_dir [use-f32]\n")
    print("  ftype == 0 -> float32")
    print("  ftype == 1 -> float16")
    sys.exit(1)


# output in the same directory as the model
model_name = sys.argv[1]
out_dir = sys.argv[2]
if not os.path.exists(out_dir):
    os.mkdir(out_dir)

fname_out = sys.argv[2] + "/ggml-replit-code-v1-3b.bin"

if not os.path.exists(model_name):
    hf_hub_download(repo_id=model_name, filename="config.json", local_dir=out_dir)
    hf_hub_download(repo_id=model_name, filename="spiece.model", local_dir=out_dir)
else:
    # copy file from model_name to out_dir
    os.system("cp " + model_name + "/config.json " + out_dir)
    os.system("cp " + model_name + "/spiece.model " + out_dir)


with open(out_dir + "/config.json", "r", encoding="utf-8") as f:
    hparams = json.load(f)

sp_proto = model.ModelProto()
sp_proto.ParseFromString(open(Path(out_dir) / "spiece.model", "rb").read())


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
    fname_out = sys.argv[1] + "/ggml-replit-code-v1-3b-" + ftype_str[ftype] + ".bin"


model = AutoModelForCausalLM.from_pretrained(
    model_name, low_cpu_mem_usage=True, trust_remote_code=True
)
# print (model)

# print(tokenizer.encode('I believe the meaning of life is'))

list_vars = model.state_dict()
for name in list_vars.keys():
    print(name, list_vars[name].shape, list_vars[name].dtype)

fout = open(fname_out, "wb")

print(hparams)

fout.write(struct.pack("i", 0x7265706c))  # magic: repl in hex
fout.write(struct.pack("i", hparams["vocab_size"]))
fout.write(struct.pack("i", hparams["max_seq_len"]))
fout.write(struct.pack("i", hparams["d_model"]))
fout.write(struct.pack("i", hparams["n_heads"]))
fout.write(struct.pack("i", hparams["n_layers"]))
fout.write(struct.pack("i", ftype))


# TODO: temporary hack to not deal with implementing the tokenizer
for piece in sp_proto.pieces:
    encoded_piece = piece.piece.encode("utf-8")
    fout.write(struct.pack("i", len(encoded_piece)))
    fout.write(encoded_piece)
    fout.write(struct.pack("f", piece.score))

name_mapping = {
    "norm_1": "ln_1",
    "norm_2": "ln_2",
    "ffn": "mlp",
    "up_proj": "mlp_up",
    "down_proj": "mlp_down",
    "norm_f":  "ln_f"
}
for name in list_vars.keys():
    data = list_vars[name].squeeze().numpy()
    print("Processing variable: " + name + " with shape: ", data.shape)

    n_dims = len(data.shape)

    # ftype == 0 -> float32, ftype == 1 -> float16
    ftype_cur = 0
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

    for x in ["norm_1", "norm_2", "ffn", "up_proj", "down_proj", "norm_f"]:
        if x in name:
            name = name.replace(x, name_mapping[x])
            print("  Renaming to: " + name)
    # header
    str = name.encode("utf-8")
    fout.write(struct.pack("iii", n_dims, len(str), ftype_cur))
    for i in range(n_dims):
        fout.write(struct.pack("i", data.shape[n_dims - 1 - i]))
    fout.write(str)

    # data
    data.tofile(fout)

fout.close()

print("Done. Output file: " + fname_out)
print("")
