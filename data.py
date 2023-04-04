import glob
import torch
from datasets import load_dataset, concatenate_datasets
import os
from torch.utils.data import DataLoader
from transformers import DefaultDataCollator



def tokenize_inputs(config, tokenizer, examples):
    max_length = config["max_length"]
    input_ids = torch.full((len(examples["prompt"]), max_length), tokenizer.pad_token_id)
    # ignore bos
    newline_tokens = tokenizer("\n", return_tensors="pt")["input_ids"][0]
    if newline_tokens[0] == tokenizer.bos_token_id:
        newline_tokens = newline_tokens[1:]

    # hacky backward compatible
    different_eos = tokenizer.eos_token != "</s>"
    out = {"labels": [], "input_ids": []}
    for prompt, response in zip(examples["prompt"], examples["response"]):
        if different_eos:
            if response.count("</s>") > 0:
                response = response.replace("</s>", tokenizer.eos_token)

        prompt_len = len(tokenizer(prompt, truncation=True, return_tensors="pt")["input_ids"][0])

        # hack if our prompt is super long
        # we need to include some labels
        if prompt_len >= max_length - 1:
            prompt = prompt[:len(prompt) // 2]

        input_tokens = tokenizer(prompt + "\n" + response + tokenizer.eos_token,
                                 truncation=True, max_length=max_length, return_tensors="pt")["input_ids"].squeeze()


        labels = input_tokens.clone()
        labels[:prompt_len + len(newline_tokens)] = -100
        if len(labels) < max_length:
            # pad to max_length with -100
            labels = torch.cat([labels, torch.full((max_length - len(labels),), -100)])

        input_tokens = tokenizer.pad({"input_ids": input_tokens}, padding="max_length", max_length=max_length)["input_ids"]
        out["labels"].append(labels)
        out["input_ids"].append(input_tokens)

    out = {k: torch.stack(v) if isinstance(v, list) else v for k, v in out.items()}

    return out


def load_data(config, tokenizer):
    dataset_path = config["dataset_path"]

    if os.path.exists(dataset_path):
        # check if path is a directory
        if os.path.isdir(dataset_path):
            files = glob.glob(os.path.join(dataset_path, "*_clean.jsonl"))
        else:
            files = [dataset_path]

        print(f"Reading files {files}")

        dataset = load_dataset("json", data_files=files, split="train")

    else:
        dataset = load_dataset(dataset_path)

    dataset = dataset.train_test_split(test_size=.05, seed=config["seed"])

    train_dataset, val_dataset = dataset["train"], dataset["test"]

    if config["streaming"] is False:
        kwargs = {"num_proc": config["num_proc"]}
    else:
        kwargs = {}

    # tokenize inputs and return labels and attention mask
    train_dataset = train_dataset.map(
        lambda ele: tokenize_inputs(config, tokenizer, ele),
        batched=True,
        remove_columns=["source", "prompt"],
        **kwargs
    )
    val_dataset = val_dataset.map(
        lambda ele: tokenize_inputs(config, tokenizer, ele), 
        batched=True,
        remove_columns=["source", "prompt"],
        **kwargs
    )

    train_dataset = train_dataset.with_format("torch")
    val_dataset = val_dataset.with_format("torch")

    # create dataloader with default data collator since we already have labels

    train_dataloader = DataLoader(
        train_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
    )

    val_dataloader = DataLoader(
        val_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
    )

    return train_dataloader, val_dataloader
