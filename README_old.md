<h1 align="center">GPT4All</h1>
<p align="center">Demo, data, and code to train open-source assistant-style large language model based on GPT-J and LLaMa</p>
<p align="center">
<a href="https://static.nomic.ai/gpt4all/2023_GPT4All-J_Technical_Report_2.pdf">:green_book: Technical Report 2: GPT4All-J </a>
</p>

<p align="center">
<a href="https://s3.amazonaws.com/static.nomic.ai/gpt4all/2023_GPT4All_Technical_Report.pdf">:green_book: Technical Report 1: GPT4All</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/pyllamacpp">:snake: Official Python Bindings</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/gpt4all-ts">:computer: Official Typescript Bindings</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/gpt4all-ui">:speech_balloon: Official Web Chat Interface</a>
</p>

<p align="center">
<a href="https://github.com/nomic-ai/gpt4all-chat">:speech_balloon: Official Chat Interface</a>
</p>

<p align="center">
<a href="https://python.langchain.com/en/latest/modules/models/llms/integrations/gpt4all.html">🦜️🔗 Official Langchain Backend</a> 
</p>


<p align="center">
<a href="https://discord.gg/mGZE39AS3e">Discord</a>
</p>




<p align="center">
GPT4All is made possible by our compute partner <a href="https://www.paperspace.com/">Paperspace</a>.
</p>



## GPT4All-J: An Apache-2 Licensed GPT4All Model
![gpt4all-j-demo](https://user-images.githubusercontent.com/13879686/231876409-e3de1934-93bb-4b4b-9013-b491a969ebbc.gif)

Run on an M1 Mac (not sped up!)


### GPT4All-J Chat UI Installers
Installs a native chat-client with auto-update functionality that runs on your desktop with the GPT4All-J model baked into it.

[Mac/OSX](https://gpt4all.io/installers/gpt4all-installer-darwin.dmg)

[Windows](https://gpt4all.io/installers/gpt4all-installer-win64.exe)

[Ubuntu](https://gpt4all.io/installers/gpt4all-installer-linux.run)

If you have older hardware that only supports avx and not avx2 you can use these.

[Mac/OSX - avx-only](https://gpt4all.io/installers/gpt4all-installer-darwin-avx-only.dmg)

[Windows - avx-only](https://gpt4all.io/installers/gpt4all-installer-win64-avx-only.exe)

[Ubuntu - avx-only](https://gpt4all.io/installers/gpt4all-installer-linux-avx-only.run)

These files are not yet cert signed by Windows/Apple so you will see security warnings on initial installation. We did not want to delay release while waiting for their process to complete.

Find the most up-to-date information on the [GPT4All Website](https://gpt4all.io/)

### Raw Model
[ggml Model Download Link](https://gpt4all.io/models/ggml-gpt4all-j.bin)

Note this model is only compatible with the C++ bindings found [here](https://github.com/nomic-ai/gpt4all-chat). It will not work with any existing llama.cpp bindings as we had to do a large fork of llama.cpp. GPT4All will support the ecosystem around this new C++ backend going forward.

Python bindings are imminent and will be integrated into this [repository](https://github.com/nomic-ai/pyllamacpp). Stay tuned on the [GPT4All discord](https://discord.gg/mGZE39AS3e) for updates.

## Training GPT4All-J

Please see [GPT4All-J Technical Report](https://static.nomic.ai/gpt4all/2023_GPT4All-J_Technical_Report_2.pdf) for details.

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
model = AutoModelForCausalLM.from_pretrained("nomic-ai/gpt4all-j-prompt-generations", revision="v1.2-jazzy")
```

### GPT4All-J Training Instructions

```bash
accelerate launch --dynamo_backend=inductor --num_processes=8 --num_machines=1 --machine_rank=0 --deepspeed_multinode_launcher standard --mixed_precision=bf16  --use_deepspeed --deepspeed_config_file=configs/deepspeed/ds_config_gptj.json train.py --config configs/train/finetune_gptj.yaml
```


# Original GPT4All Model (based on GPL Licensed LLaMa)



![gpt4all-lora-demo](https://user-images.githubusercontent.com/13879686/228352356-de66ca7a-df70-474e-b929-2e3656165051.gif)

Run on M1 Mac (not sped up!)

# Try it yourself

Here's how to get started with the CPU quantized GPT4All model checkpoint:

1. Download the `gpt4all-lora-quantized.bin` file from [Direct Link](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized.bin) or [[Torrent-Magnet]](https://tinyurl.com/gpt4all-lora-quantized).
2. Clone this repository, navigate to `chat`, and place the downloaded file there.
3. Run the appropriate command for your OS:
   - M1 Mac/OSX: `cd chat;./gpt4all-lora-quantized-OSX-m1`
   - Linux: `cd chat;./gpt4all-lora-quantized-linux-x86`
   - Windows (PowerShell): `cd chat;./gpt4all-lora-quantized-win64.exe`
   - Intel Mac/OSX: `cd chat;./gpt4all-lora-quantized-OSX-intel`

For custom hardware compilation, see our [llama.cpp](https://github.com/zanussbaum/gpt4all.cpp) fork.

-----------
Find all compatible models in the GPT4All Ecosystem section.

[Secret Unfiltered Checkpoint](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-unfiltered-quantized.bin) - [[Torrent]](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-unfiltered-quantized.bin.torrent)

This model had all refusal to answer responses removed from training. Try it with:
- M1 Mac/OSX: `cd chat;./gpt4all-lora-quantized-OSX-m1 -m gpt4all-lora-unfiltered-quantized.bin`
- Linux: `cd chat;./gpt4all-lora-quantized-linux-x86 -m gpt4all-lora-unfiltered-quantized.bin`
- Windows (PowerShell): `cd chat;./gpt4all-lora-quantized-win64.exe -m gpt4all-lora-unfiltered-quantized.bin`
- Intel Mac/OSX: `cd chat;./gpt4all-lora-quantized-OSX-intel -m gpt4all-lora-unfiltered-quantized.bin`
-----------
Note: the full model on GPU (16GB of RAM required) performs much better in our qualitative evaluations.

# Python Client
## CPU Interface
To run GPT4All in python, see the new [official Python bindings](https://github.com/nomic-ai/pyllamacpp).

The old bindings are still available but now deprecated. They will not work in a notebook environment.
To get running using the python client with the CPU interface, first install the [nomic client](https://github.com/nomic-ai/nomic) using `pip install nomic`
Then, you can use the following script to interact with GPT4All:
```
from nomic.gpt4all import GPT4All
m = GPT4All()
m.open()
m.prompt('write me a story about a lonely computer')
```

## GPU Interface
There are two ways to get up and running with this model on GPU.
The setup here is slightly more involved than the CPU model.
1. clone the nomic client [repo](https://github.com/nomic-ai/nomic) and run `pip install .[GPT4All]` in the home dir.
2. run `pip install nomic` and install the additional deps from the wheels built [here](https://github.com/nomic-ai/nomic/tree/main/bin)

Once this is done, you can run the model on GPU with a script like the following:
```
from nomic.gpt4all import GPT4AllGPU
m = GPT4AllGPU(LLAMA_PATH)
config = {'num_beams': 2,
          'min_new_tokens': 10,
          'max_length': 100,
          'repetition_penalty': 2.0}
out = m.generate('write me a story about a lonely computer', config)
print(out)
```
Where LLAMA_PATH is the path to a Huggingface Automodel compliant LLAMA model.
Nomic is unable to distribute this file at this time.
We are working on a GPT4All that does not have this limitation right now.

You can pass any of the [huggingface generation config params](https://huggingface.co/docs/transformers/main_classes/text_generation#transformers.GenerationConfig) in the config.

# GPT4All Compatibility Ecosystem
Edge models in the GPT4All Ecosystem. Please PR as the [community grows](https://huggingface.co/models?sort=modified&search=4bit).
Feel free to convert this to a more structured table.

- [gpt4all](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized.bin) [[MD5 Signature](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized.bin.md5)]
   - [gpt4all-ggml-converted](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized-ggml.bin) [[MD5 Signature](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized-ggml.bin.md5)]
- [gpt4all-unfiltered](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-unfiltered-quantized.bin) [[MD5 Signature](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-unfiltered-quantized.bin.md5)]
- [ggml-vicuna-7b-4bit](https://huggingface.co/eachadea/ggml-vicuna-7b-4bit)
- [vicuna-13b-GPTQ-4bit-128g](https://huggingface.co/anon8231489123/vicuna-13b-GPTQ-4bit-128g)
- [LLaMa-Storytelling-4Bit](https://huggingface.co/GamerUntouch/LLaMa-Storytelling-4Bit)
- [Alpaca Native 4bit](https://huggingface.co/Sosaka/Alpaca-native-4bit-ggml/tree/main)


# Roadmap
## Short Term
 - <span style="color:green">(Done)</span> Train a GPT4All model based on GPTJ to alleviate llama distribution issues.
 - <span style="color:green">(Done)</span> Create improved CPU and GPU interfaces for this model.
 - <span style="color:green">(Done)</span> [Integrate llama.cpp bindings](https://github.com/nomic-ai/pyllamacpp)
 - <span style="color:green">(Done)</span> [Create a good conversational chat interface for the model.](https://github.com/nomic-ai/gpt4all-ui)
 - <span style="color:green">(Done)</span> [Allow users to opt in and submit their chats for subsequent training runs](https://github.com/nomic-ai/gpt4all-ui)

## Medium Term
 - <span style="color:red">(NOT STARTED)</span> Integrate GPT4All with [Atlas](https://atlas.nomic.ai) to allow for document retrieval.
   - BLOCKED by GPT4All based on GPTJ
 - <span style="color:red">(Done)</span> Integrate GPT4All with Langchain.
 - <span style="color:green">(IN PROGRESS)</span> Build easy custom training scripts to allow users to fine tune models.

## Long Term
 - <span style="color:red">(NOT STARTED)</span> Allow anyone to curate training data for subsequent GPT4All releases using Atlas.
 - <span style="color:green">(IN PROGRESS)</span> Democratize AI. 

# Reproducibility

Trained Model Weights:
- gpt4all-lora (four full epochs of training):  https://huggingface.co/nomic-ai/gpt4all-lora
- gpt4all-lora-epoch-2 (three full epochs of training) https://huggingface.co/nomic-ai/gpt4all-lora-epoch-2
- gpt4all-j (one full epoch of training) (https://huggingface.co/nomic-ai/gpt4all-j)
- gpt4all-j-lora (one full epoch of training) (https://huggingface.co/nomic-ai/gpt4all-j-lora)

Raw Data:
- [Training Data Without P3](https://huggingface.co/datasets/nomic-ai/gpt4all_prompt_generations)
  - Explorer: https://atlas.nomic.ai/map/gpt4all_data_clean_without_p3
- [Full Dataset with P3](https://huggingface.co/datasets/nomic-ai/gpt4all_prompt_generations_with_p3)
  - Explorer: https://atlas.nomic.ai/map/gpt4all_data_clean
- [GPT4All-J Dataset](https://huggingface.co/datasets/nomic-ai/gpt4all-j-prompt-generations)
   -  Explorer Indexed on Prompts: https://atlas.nomic.ai/map/gpt4all-j-prompts-curated
   -  Exporer Indexed on Responses: https://atlas.nomic.ai/map/gpt4all-j-response-curated

We are not distributing a LLaMa 7B checkpoint.

You can reproduce our trained model by doing the following:

## Setup

Clone the repo

```
git clone --recurse-submodules https://github.com/nomic-ai/gpt4all.git
git submodule update --init
```

Setup the environment

```
python -m pip install -r requirements.txt

cd ../peft
pip install -e .
```

## Training

```bash
accelerate launch --dynamo_backend=inductor --num_processes=8 --num_machines=1 --machine_rank=0 --deepspeed_multinode_launcher standard --mixed_precision=bf16  --use_deepspeed --deepspeed_config_file=configs/deepspeed/ds_config.json train.py --config configs/train/finetune-7b.yaml
```

## Generate

```bash
python generate.py --config configs/generate/generate.yaml --prompt "Write a script to reverse a string in Python"
```

## Need Help?

Join the <a href="https://discord.gg/E7q6fp9bmx"> Discord </a> and ask for help in `#gpt4all-help`

# Sample Generations

### Provide instructions for the given exercise. Leg Raises
>Leg Raises
>1. Stand with your feet shoulder-width apart and your knees slightly bent.
>2. Raise your arms straight out in front of you.
>3. Slowly bend your knees and raise your heels off the ground.
>4. Hold this position for a count of 3, then slowly lower your heels back to the ground.
>5. Repeat the exercise 10 times.
>6. Do not let your knees go beyond your toes.
>7. Keep your back straight and your abdominal muscles engaged.
>8. Do not let your hips sag or your back arch.
>9. Keep your arms straight and your elbows locked.
>10. Do not let your head drop or your shoulders rise.
>11. Breathe normally throughout the exercise.
>12. Rest for 30 seconds before repeating the exercise.
>13. Gradually increase the number of repetitions as you become stronger.


### A color description has been provided. Find the CSS code associated with that color. A light red color with a medium light shade of pink

>The CSS code associated with the color provided is: #FF6347


### Come up with an interesting idea for a new movie plot. Your plot should be described with a title and a summary.

> 1. The title of your movie plot should be "The Last Stand". 
> 2. The summary of your movie plot should be: A group of mercenaries are hired to protect a small town from a dangerous criminal who is on the run. However, the town's sheriff is not willing to let them take over and decides to take matters into his own hands.

### Reverse a string in python.
> In Python, you can reverse a list or tuple by using the reversed() function on it. Here's an example of how to use this method with strings: 
>    ```python
>   my_string = "Hello World" # Define your original string here
>   reversed_str = my_string[::-1]   # Use a slice and negative index for reverse order
>   print(reversed_str)     # Output will be reversed version of the inputted string.

### List 10 dogs.
> 1. Labrador Retriever
> 2. Golden Retriever
> 3. Beagle
> 4. Poodle
> 5. German Shepherd
> 6. Bulldog
> 7. Rottweiler
> 8. Boxer
> 9. Shih Tzu
> 10. Pug

### Write me a poem about the fall of Julius Caesar into a caesar salad in iambic pentameter. 

> The fall of Julius Caesar into a caesar salad<br>
> In iambic pentameter,<br>
> The mighty Roman emperor<br>
> Fell into a salad of lettuce and croutons,<br>
> His empire crumbling around him,<br>
> As he was devoured by the hungry diners.<br>
> The once mighty emperor<br>
> Was now just a salad topping,<br>
> His legacy reduced to a mere garnish,<br>
> As he was consumed by the masses.<br>
> The fall of Julius Caesar into a caesar salad,<br>
> In iambic pentameter,<br>
> Was a tragic end to a great man,<br>
> Who had once ruled the world with an iron fist.<br>
> But now, he was just a salad topping,<br>
> His legacy reduced to a mere garnish,<br>
> As he was consumed by the masses.
    
### What is a three word topic describing the following keywords: baseball, football, soccer: 
> Sports, athletics, games
    
## Citation

If you utilize this repository, models or data in a downstream project, please consider citing it with:
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
