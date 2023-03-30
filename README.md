<h1 align="center">GPT4All</h1>
<p align="center">Demo, data and code to train an assistant-style large language model with ~800k GPT-3.5-Turbo Generations based on LLaMa</p>

<p align="center">
<a href="https://s3.amazonaws.com/static.nomic.ai/gpt4all/2023_GPT4All_Technical_Report.pdf">:green_book: Technical Report</a>
</p>
<p align="center">
<a href="https://discord.gg/kvmy6dQB">Discord</a>
</p>



![gpt4all-lora-demo](https://user-images.githubusercontent.com/13879686/228352356-de66ca7a-df70-474e-b929-2e3656165051.gif)

Run on M1 Mac (not sped up!)

# Try it yourself

Download the CPU quantized gpt4all model checkpoint: [gpt4all-lora-quantized.bin](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized.bin) - [[Torrent-Magnet]](https://tinyurl.com/gpt4all-lora-quantized) 


Clone this repository down and place the quantized model in the `chat` directory and start chatting by running:

- `cd chat;./gpt4all-lora-quantized-OSX-m1` on M1 Mac/OSX
- `cd chat;./gpt4all-lora-quantized-linux-x86` on Linux
- `cd chat;./gpt4all-lora-quantized-win64.exe` on Windows (PowerShell)
- `cd chat;./gpt4all-lora-quantized-OSX-intel` on Intel Mac/OSX

To compile for custom hardware, see our fork of the [Alpaca C++](https://github.com/zanussbaum/gpt4all.cpp) repo.

-----------

[Secret Unfiltered Checkpoint](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-unfiltered-quantized.bin) - [[Torrent]](https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-unfiltered-quantized.bin.torrent)

This model had all refusal to answer responses removed from training. Try it with:
- `cd chat;./gpt4all-lora-quantized-OSX-m1 -m gpt4all-lora-unfiltered-quantized.bin`

-----------
Note: the full model on GPU (16GB of RAM required) performs much better in our qualitative evaluations.

# Reproducibility

Trained LoRa Weights:
- gpt4all-lora (four full epochs of training):  https://huggingface.co/nomic-ai/gpt4all-lora
- gpt4all-lora-epoch-2 (three full epochs of training) https://huggingface.co/nomic-ai/gpt4all-lora-epoch-2

Raw Data:
- [Training Data Without P3](https://s3.amazonaws.com/static.nomic.ai/gpt4all/2022_03_27/gpt4all_curated_data_without_p3_2022_03_27.tar.gz)
  - Explorer: https://atlas.nomic.ai/map/gpt4all_data_clean_without_p3
- [Full Dataset with P3](https://s3.amazonaws.com/static.nomic.ai/gpt4all/2022_03_27/gpt4all_curated_data_full_2022_03_27.tar.gz)
  - Explorer: https://atlas.nomic.ai/map/gpt4all_data_clean

We are not distributing a LLaMa 7B checkpoint.

You can reproduce our trained model by doing the following:

## Setup

Clone the repo

`git clone --recurse-submodules https://github.com/nomic-ai/gpt4all.git`

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
python generate.py --config configs/generate/generate.yaml --prompt "Write a script to reverse a string in Python"
```

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

>1. The title of your movie plot should be "The Last Stand". 2. The summary of your movie plot should be: A group of mercenaries are hired to protect a small town from a dangerous criminal who is on the run. However, the town's sheriff is not willing to let them take over and decides to take matters into his own hands.

### Reverse a string in python.
> In Python, you can reverse a list or tuple by using the reversed() function on it. Here's an example of how to use this method with strings: 
>    ```python
>   my_string = "Hello World" # Define your original string here
>   reversed_str = my_string[::-1]   # Use a slice and negative index for reverse order
>   print(reversed_str)     # Output will be reversed version of the inputted string.

### List 10 dogs.
>1. Labrador Retriever
>2. Golden Retriever
>3. Beagle
>4. Poodle
>5. German Shepherd
>6. Bulldog
>7. Rottweiler
>8. Boxer
>9. Shih Tzu
>10. Pug

### Write me a poem about the fall of Julius Ceasar into a ceasar salad in iambic pentameter. 
>The fall of Julius Ceasar into a ceasar salad
>In iambic pentameter,
>The mighty Roman emperor
>Fell into a salad of lettuce and croutons,
>His empire crumbling around him,
>As he was devoured by the hungry diners.
>The once mighty emperor
>Was now just a salad topping,
>His legacy reduced to a mere garnish,
>As he was consumed by the masses.
>The fall of Julius Ceasar into a ceasar salad,
>In iambic pentameter,
>Was a tragic end to a great man,
>Who had once ruled the world with an iron fist.
>But now, he was just a salad topping,
>His legacy reduced to a mere garnish,
>As he was consumed by the masses.
    
### What is a three word topic describing the following keywords: baseball, football, soccer: 
>Sports, athletics, games
    
### GPU Interface
There are two ways to get up and running with this model on GPU.
1. clone the nomic client [repo](https://github.com/nomic-ai/nomic) and run `pip install .[GPT4All]` in the home dir.
2. run `pip install nomic` and install the additional deps from the wheels built [here](https://github.com/nomic-ai/nomic/tree/main/bin)

Once this is done, you can run the model on GPU with a script like the following:
```
from nomic import GPT4AllGPU
m = GPT4AllGPU(LLAMA_PATH)
config = {'num_beams': 2,
          'min_new_tokens': 10,
          'max_length': 100,
          'repetition_penalty': 2.0}
out = m.generate('write me a story about a lonely computer', config)
print(out)
```
You can pass any of the [huggingface generation config params](https://huggingface.co/docs/transformers/main_classes/text_generation#transformers.GenerationConfig) in the config.

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
