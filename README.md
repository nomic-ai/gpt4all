<h1 align="center">GPT4All</h1>
<p align="center">Demo, data and code to train an assistant-style large language model</p>

<p align="center">
<a target="_blank" href="https://s3.amazonaws.com/static.nomic.ai/gpt4all/2023_GPT4All_Technical_Report.pdf">:green_book: Technical Report</a>
</p>



![gpt4all-lora-demo](https://user-images.githubusercontent.com/13879686/228352356-de66ca7a-df70-474e-b929-2e3656165051.gif)


# Try it yourself
You can download pre-compiled LLaMa C++ Interactive Chat binaries here:
- [OSX](https://s3.amazonaws.com/static.nomic.ai/gpt4all/models/gpt4all-lora-quantized-OSX-m1)
- [Intel/Windows]()

and the model
- [gpt4all-quantized]()




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

## Training

```bash
accelerate launch --dynamo_backend=inductor --num_processes=8 --num_machines=1 --machine_rank=0 --deepspeed_multinode_launcher standard --mixed_precision=bf16  --use_deepspeed --deepspeed_config_file=configs/deepspeed/ds_config.json train.py --config configs/train/finetune-7b.yaml
```

## Generate

```bash
python generate.py --config configs/generate/generate.yaml --prompt "Write a script to reverse a string in Python
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

