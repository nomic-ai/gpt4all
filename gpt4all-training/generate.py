#!/usr/bin/env python3
from transformers import AutoModelForCausalLM, AutoTokenizer
from peft import PeftModelForCausalLM
from read import read_config
from argparse import ArgumentParser
import torch
import time


def generate(tokenizer, prompt, model, config):
    input_ids = tokenizer(prompt, return_tensors="pt").input_ids.to(model.device)

    outputs = model.generate(input_ids=input_ids, max_new_tokens=config["max_new_tokens"], temperature=config["temperature"])

    decoded = tokenizer.decode(outputs[0], skip_special_tokens=True).strip()

    return decoded[len(prompt):]

    
def setup_model(config):
    model = AutoModelForCausalLM.from_pretrained(config["model_name"], device_map="auto", torch_dtype=torch.float16)
    tokenizer = AutoTokenizer.from_pretrained(config["tokenizer_name"])
    added_tokens = tokenizer.add_special_tokens({"bos_token": "<s>", "eos_token": "</s>", "pad_token": "<pad>"})

    if added_tokens > 0:
        model.resize_token_embeddings(len(tokenizer))

    if config["lora"]:
        model = PeftModelForCausalLM.from_pretrained(model, config["lora_path"], device_map="auto", torch_dtype=torch.float16)
        model.to(dtype=torch.float16)

    print(f"Mem needed: {model.get_memory_footprint() / 1024 / 1024 / 1024:.2f} GB")
        
    return model, tokenizer

    
    
if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--config", type=str, required=True)
    parser.add_argument("--prompt", type=str)

    args = parser.parse_args()

    config = read_config(args.config)

    if config["prompt"] is None and args.prompt is None:
        raise ValueError("Prompt is required either in config or as argument")

    prompt = config["prompt"] if args.prompt is None else args.prompt

    print("Setting up model")
    model, tokenizer = setup_model(config)

    print("Generating")
    start = time.time()
    generation = generate(tokenizer, prompt, model, config)
    print(f"Done in {time.time() - start:.2f}s")
    print(generation)
