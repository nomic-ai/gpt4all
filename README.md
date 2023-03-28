<h1 align="center">GPT4All</h1>
<p align="center">Data and code to train an assistant-style LLM</p>

# Try it yourself

-- TODO LLAMA C++ code



# Reproducibility
To reproduce our trained LoRa model, do the following:

## Setup

Clone the repo

`git clone --recurse-submodules git@github.com:nomic-ai/gpt4all.git`

`git submodule configure && git submodule update`

Setup the environment

```
python -m pip install -r requirements.txt

cd transformers
pip install -e . 

cd ../peft
pip install -e .
```


## Generate

`python generate.py --config configs/generate/generate.yaml --prompt "Write a script to reverse a string in Python`


## Train

`accelerate launch --dynamo_backend=inductor --num_processes=8 --num_machines=1 --machine_rank=0 --deepspeed_multinode_launcher standard --mixed_precision=bf16  --use_deepspeed --deepspeed_config_file=configs/deepspeed/ds_config.json train.py --config configs/train/finetune-7b.yaml`
