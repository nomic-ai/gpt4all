<h1 align="center">GPT4All</h1>
<p align="center">Demo, data and code to train an assistant-style large language model</p>

# Try it yourself

-- TODO LLAMA C++ code



# Reproducibility

You can find trained LoRa model weights at:
- gpt4all-lora https://huggingface.co/nomic-ai/gpt4all-lora

We are not distributing a LLaMa 7B checkpoint.

You can reproduce our trained model by doing the following:

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

```bash
python generate.py --config configs/generate/generate.yaml --prompt "Write a script to reverse a string in Python
```


## Train

```bash
accelerate launch --dynamo_backend=inductor --num_processes=8 --num_machines=1 --machine_rank=0 --deepspeed_multinode_launcher standard --mixed_precision=bf16  --use_deepspeed --deepspeed_config_file=configs/deepspeed/ds_config.json train.py --config configs/train/finetune-7b.yaml
```



If you utilize this reposistory, models or data in a downstream project, please consider citing it with:
```
@misc{gpt4all,
  author = {Yuvanesh Anand and Zach Nussbaum and Brandon Duderstadt and Benjamin Schmidt and Andriy Mulyar},
  title = {GPT4All: Training an Assistant-style Chatbot with Large Scale Data Distillation from GPT-3.5-Turbo},
  year = {2023},
  publisher = {GitHub},
  journal = {GitHub repository},
  howpublished = {\url{https://github.com/nomic-ai/gpt4all}},
}
```

