import json
import torch
import numpy as np
from read import read_config
from argparse import ArgumentParser
from peft import PeftModelForCausalLM
from transformers import AutoModelForCausalLM, AutoTokenizer

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

    if config["lora"]:
        model = PeftModelForCausalLM.from_pretrained(model, config["lora_path"], device_map="auto", torch_dtype=torch.float16, return_hidden_states=True)
        model.to(dtype=torch.float16)

    print(f"Mem needed: {model.get_memory_footprint() / 1024 / 1024 / 1024:.2f} GB")
        
    return model, tokenizer





def eval_example(model, tokenizer, example, config):

    #set up data
    prompt = example['instruction'] + ' ' + example['instances'][0]['input']
    gt = prompt + ' ' + example['instances'][0]['output']

    #decode several continuations and compute their page trajectories
    input = tokenizer(prompt, return_tensors="pt")
    input = {k: v.to(model.device) for k, v in input.items()}

    continuations = []
    trajectories = []
    for i in range(5):
        print(i)
        outputs = model.generate(input_ids=input['input_ids'],
                                 max_new_tokens=config["max_new_tokens"],
                                 temperature=config["temperature"])

        y = model(input_ids=outputs)
        trajectory = y.hidden_states[0].detach().cpu().numpy()[0]
        trajectory = trajectory / np.linalg.norm(trajectory, axis=1, keepdims=True)
        trajectory = np.cumsum(trajectory, axis=0) / np.arange(1, trajectory.shape[0]+1).reshape(-1, 1)
        decoded = tokenizer.decode(outputs[0], skip_special_tokens=True).strip()

        trajectories.append(trajectory)
        continuations.append(decoded[len(prompt):])

    #compute the ground truth perplexity
    nlls = []
    prev_end_loc = 0
    for begin_loc in tqdm(range(len(prompt), len(gt), 1)):
        end_loc = min(begin_loc + max_length, seq_len)
        trg_len = end_loc - prev_end_loc  # may be different from stride on last loop
        input_ids = input['input_ids'][:, begin_loc:end_loc].to(model.device)
        target_ids = input_ids.clone()
        target_ids[:, :-trg_len] = -100

        with torch.no_grad():
            outputs = model(input_ids, labels=target_ids)
            neg_log_likelihood = outputs.loss * trg_len

        nlls.append(neg_log_likelihood)
        prev_end_loc = end_loc
        if end_loc == seq_len:
            break

    ppl = torch.exp(torch.stack(nlls).sum() / end_loc)

    print('perplexity: ', ppl)
    print('trajectories: ', trajectories)
    print('continuations: ', continuations)

    raise

    return ppl, trajectories, continuations

def do_eval(config):
    eval_data = read_jsonl_file('eval_data/user_oriented_instructions.jsonl')
    model, tokenizer = setup_model(config)
    trajectories = []
    perplexities = []
    continuations = []
    for example in eval_data:
        gt_perplexity, trajectories, continuations = eval_example(model, tokenizer, example, config)




if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("--config", type=str, required=True)
    args = parser.parse_args()

    config = read_config(args.config)
    do_eval(config)
