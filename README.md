<h1 align="center">GPT4All</h1>
<p align="center">Demo, data and code to train an assistant-style large language model on ~440k GPT-3.5-Turbo Generations</p>

<p align="center">
<a href="https://s3.amazonaws.com/static.nomic.ai/gpt4all/2023_GPT4All_Technical_Report.pdf">:green_book: Technical Report</a>
</p>



![gpt4all-lora-demo](https://user-images.githubusercontent.com/13879686/228352356-de66ca7a-df70-474e-b929-2e3656165051.gif)


# Try it yourself
You can download pre-compiled GPT4ALL Interactive Chat binaries here:
- [OSX](https://s3.amazonaws.com/static.nomic.ai/gpt4all/models/gpt4all-lora-quantized-OSX-m1)
- [Intel/Windows]()

and the model
- [gpt4all-quantized](https://s3.amazonaws.com/static.nomic.ai/gpt4all/models/gpt4all-lora-quantized.bin)

Place the binary and quantized model in the same directory and start chatting!

To compile for custom hardware, see our fork of the [Alpaca C++](https://github.com/zanussbaum/gpt4all.cpp) repo.


# Reproducibility

Trained LoRa Weights:
- gpt4all-lora:  https://huggingface.co/nomic-ai/gpt4all-lora
- gpt4all-lora-epoch-2 https://huggingface.co/nomic-ai/gpt4all-lora-epoch-2

Raw Data:
- [Training Data Without P3](https://s3.amazonaws.com/static.nomic.ai/gpt4all/2022_03_27/gpt4all_curated_data_without_p3_2022_03_27.tar.gz)
- [Full Dataset with P3](https://s3.amazonaws.com/static.nomic.ai/gpt4all/2022_03_27/gpt4all_curated_data_full_2022_03_27.tar.gz)

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

