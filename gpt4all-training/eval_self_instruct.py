#!/usr/bin/env python3
import json
import torch
import pickle
import numpy as np
from tqdm import tqdm
from read import read_config
from argparse import ArgumentParser
from peft import PeftModelForCausalLM
from transformers import AutoModelForCausalLM, AutoTokenizer

'''
Evaluates perplexity on the outputs of:
https://github.com/yizhongw/self-instruct/blob/main/human_eval/user_oriented_instructions.jsonl
'''

def read_jsonl_file(file_path):
    data = []
    with open(file_path, 'r', encoding='utf-8') as file:
        for line in file:
            json_object = json.loads(line.strip())
            data.append(json_object)
    return data

def setup_model(config):
    model = AutoModelForCausalLM.from_pretrained(config["model_name"], device_map="auto", torch_dtype=torch.float16, output_hidden_states=True)
    tokenizer = AutoTokenizer.from_pretrained(config["tokenizer_name"])
    added_tokens = tokenizer.add_special_tokens({"bos_token": "<s>", "eos_token": "</s>", "pad_token": "<pad>"})

    if added_tokens > 0:
        model.resize_token_embeddings(len(tokenizer))

    if 'lora' in config and config['lora']:
        model = PeftModelForCausalLM.from_pretrained(model, config["lora_path"], device_map="auto", torch_dtype=torch.float16, return_hidden_states=True)
        model.to(dtype=torch.float16)

    print(f"Mem needed: {model.get_memory_footprint() / 1024 / 1024 / 1024:.2f} GB")
        
    return model, tokenizer




def eval_example(model, tokenizer, example, config):

    prompt = example['instruction'] + ' ' + example['instances'][0]['input']
    gt = prompt + ' ' + example['instances'][0]['output']

    #decode several continuations and compute their page trajectories
    input = tokenizer(prompt, return_tensors="pt")
    input = {k: v.to(model.device) for k, v in input.items()}

    #compute the ground truth perplexity
    gt_input = tokenizer(gt, return_tensors="pt")
    gt_input = {k: v.to(model.device) for k, v in gt_input.items()}

    nlls = []
    prev_end_loc = 0
    stride = 512
    seq_len = gt_input['input_ids'].size(1)

    for begin_loc in tqdm(range(input['input_ids'].size(1), gt_input['input_ids'].size(1), stride)):
        end_loc = min(begin_loc + stride, seq_len)
        trg_len = end_loc - prev_end_loc  # may be different from stride on last loop
        input_ids = gt_input['input_ids'][:, begin_loc:end_loc].to(model.device)
        target_ids = input_ids.clone()
        target_ids[:, :-trg_len] = -100

        with torch.no_grad():
            outputs = model(input_ids, labels=target_ids)
            neg_log_likelihood = outputs.loss * trg_len

        nlls.append(neg_log_likelihood)
        prev_end_loc = end_loc
        if end_loc == seq_len:
            break

    ppl = torch.exp(torch.stack(nlls).sum() / end_loc).item()
    print('ppl: ', ppl)

    print(prompt)
    print(80*'-')
   

    return ppl

def do_eval(config):
    eval_data = read_jsonl_file('eval_data/user_oriented_instructions.jsonl')
    model, tokenizer = setup_model(config)
    all_perplexities = []
    for example in tqdm(eval_data):
        gt_perplexity = eval_example(model, tokenizer, example, config)
        all_perplexities.append(gt_perplexity)

        
    name = f"eval_data/eval__model-{config['model_name'].replace('/', '_')}{'__lora-' + config['lora_path'].replace('/', '_') if config['lora'] else ''}.pkl"

    with open(name, 'wb') as f:
        r = {'perplexities': all_perplexities}
        pickle.dump(r, f)


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("--config", type=str, required=True)
    args = parser.parse_args()

    config = read_config(args.config)
    do_eval(config)
