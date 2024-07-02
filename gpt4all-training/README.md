## Training GPT4All-J

### Technical Reports

<p align="center">
<a href="https://gpt4all.io/reports/GPT4All_Technical_Report_3.pdf">:green_book: Technical Report 3: GPT4All Snoozy and Groovy </a>
</p>

<p align="center">
<a href="https://static.nomic.ai/gpt4all/2023_GPT4All-J_Technical_Report_2.pdf">:green_book: Technical Report 2: GPT4All-J </a>
</p>

<p align="center">
<a href="https://s3.amazonaws.com/static.nomic.ai/gpt4all/2023_GPT4All_Technical_Report.pdf">:green_book: Technical Report 1: GPT4All</a>
</p>

### GPT4All-J Training Data

- We are releasing the curated training data for anyone to replicate GPT4All-J here: [GPT4All-J Training Data](https://huggingface.co/datasets/nomic-ai/gpt4all-j-prompt-generations)
   - [Atlas Map of Prompts](https://atlas.nomic.ai/map/gpt4all-j-prompts-curated)
   - [Atlas Map of Responses](https://atlas.nomic.ai/map/gpt4all-j-response-curated)
   
We have released updated versions of our `GPT4All-J` model and training data. 

- `v1.0`: The original model trained on the v1.0 dataset
- `v1.1-breezy`: Trained on a filtered dataset where we removed all instances of AI language model
- `v1.2-jazzy`: Trained on a filtered dataset where we also removed instances like I'm sorry, I can't answer... and AI language model

The [models](https://huggingface.co/nomic-ai/gpt4all-j) and [data](https://huggingface.co/datasets/nomic-ai/gpt4all-j-prompt-generations) versions can be specified by passing a `revision` argument.

For example, to load the `v1.2-jazzy` model and dataset, run:

```python
from datasets import load_dataset
from transformers import AutoModelForCausalLM

dataset = load_dataset("nomic-ai/gpt4all-j-prompt-generations", revision="v1.2-jazzy")
model = AutoModelForCausalLM.from_pretrained("nomic-ai/gpt4all-j", revision="v1.2-jazzy")
```

### GPT4All-J Training Instructions

```bash
accelerate launch --dynamo_backend=inductor --num_processes=8 --num_machines=1 --machine_rank=0 --deepspeed_multinode_launcher standard --mixed_precision=bf16  --use_deepspeed --deepspeed_config_file=configs/deepspeed/ds_config_gptj.json train.py --config configs/train/finetune_gptj.yaml
```
