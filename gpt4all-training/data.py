import glob
import torch
from datasets import load_dataset, concatenate_datasets
import os
from torch.utils.data import DataLoader
from transformers import DefaultDataCollator



def tokenize_inputs(config, tokenizer, examples):
    max_length = config["max_length"]

    # hacky backward compatible
    different_eos = tokenizer.eos_token != "</s>"
    out = {"labels": [], "input_ids": [], "attention_mask": []}
    for prompt, response in zip(examples["prompt"], examples["response"]):
        if different_eos:
            if response.count("</s> \n") > 0:
                response = response.replace("</s> \n", f"{tokenizer.eos_token} \n") 

        prompt_len = len(tokenizer(prompt + "\n", return_tensors="pt")["input_ids"][0])

        # hack if our prompt is super long
        # we need to include some labels so we arbitrarily trunacate at max_length // 2
        # if the length is too long
        if prompt_len >= max_length // 2:
            # if prompt is too long, truncate
            # but make sure to truncate to at max 1024 tokens
            new_len = min(max_length // 2, len(prompt) // 2)
            prompt = prompt[:new_len]
            # get new prompt length
            prompt_len = tokenizer(prompt + "\n", return_tensors="pt", max_length=max_length // 2, truncation=True).input_ids.ne(tokenizer.pad_token_id).sum().item()

        assert prompt_len <= max_length // 2, f"prompt length {prompt_len} exceeds max length {max_length}"

        input_tokens = tokenizer(prompt + "\n" + response + tokenizer.eos_token,
                                 truncation=True, max_length=max_length, return_tensors="pt")["input_ids"].squeeze()

        labels = input_tokens.clone()
        labels[:prompt_len] = -100
        if len(labels) < max_length:
            # pad to max_length with -100
            labels = torch.cat([labels, torch.full((max_length - len(labels),), -100)])

        assert (labels == -100).sum() < len(labels), f"Labels are all -100, something wrong. prompt length {prompt_len} exceeds max length {max_length}" 
        
        if (labels == -100).sum() == len(labels) - 1:
            print(prompt)
            print(response)
            raise

        padded = tokenizer.pad({"input_ids": input_tokens}, padding="max_length", max_length=max_length, return_tensors="pt")
        out["labels"].append(labels)
        out["input_ids"].append(padded["input_ids"])
        out["attention_mask"].append(padded["attention_mask"])

    out = {k: torch.stack(v) if isinstance(v, list) else v for k, v in out.items()}

    return out


def load_data(config, tokenizer):
    dataset_path = config["dataset_path"]

    if os.path.exists(dataset_path):
        if os.path.isdir(dataset_path):
            files = glob.glob(os.path.join(dataset_path, "*_clean.jsonl"))
        else:
            files = [dataset_path]

        print(f"Reading files {files}")

        dataset = load_dataset("json", data_files=files, split="train")

    else:
        dataset = load_dataset(dataset_path, split="train", revision=config["revision"] if "revision" in config else None)

    dataset = dataset.train_test_split(test_size=.05, seed=config["seed"])

    train_dataset, val_dataset = dataset["train"], dataset["test"]

    if config["streaming"] is False:
        kwargs = {"num_proc": config["num_proc"]}
    else:
        kwargs = {}

    cols_to_keep = ["input_ids", "labels", "attention_mask"]
    # tokenize inputs and return labels and attention mask
    train_dataset = train_dataset.map(
        lambda ele: tokenize_inputs(config, tokenizer, ele),
        batched=True,
        **kwargs
    )
    remove_cols = [col for col in train_dataset.column_names if col not in cols_to_keep]
    train_dataset = train_dataset.remove_columns(remove_cols)

    val_dataset = val_dataset.map(
        lambda ele: tokenize_inputs(config, tokenizer, ele),
        batched=True,
        **kwargs
    )
    remove_cols = [col for col in val_dataset.column_names if col not in cols_to_keep]
    val_dataset = val_dataset.remove_columns(remove_cols)

    train_dataset = train_dataset.with_format("torch")
    val_dataset = val_dataset.with_format("torch")

    # create dataloader with default data collator since we already have labels

    train_dataloader = DataLoader(
        train_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
        shuffle=True,
    )

    val_dataloader = DataLoader(
        val_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
        shuffle=True,
    )

    return train_dataloader, val_dataloader

    
def load_data_for_inference(config, tokenizer):
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
        dataset = load_dataset(dataset_path, split="train")

    dataset = dataset.train_test_split(test_size=.05, seed=config["seed"])

    train_dataset, val_dataset = dataset["train"], dataset["test"]

    train_dataset = train_dataset.add_column("index", list(range(len(train_dataset))))
    # select first N batches that are divisible by batch_size
    # gather is a bit annoying (or the way I'm using it) to get uneven batches as it duplicates data
    train_dataset = train_dataset.select(range((len(train_dataset) // config["batch_size"]) * config["batch_size"]))
    val_dataset = val_dataset.add_column("index", list(range(len(val_dataset))))
    val_dataset = val_dataset.select(range((len(val_dataset) // config["batch_size"]) * config["batch_size"]))

    if config["streaming"] is False:
        kwargs = {"num_proc": config["num_proc"]}
    else:
        kwargs = {}

    # tokenize inputs and return labels and attention mask
    train_dataset = train_dataset.map(
        lambda ele: tokenize_inputs(config, tokenizer, ele),
        batched=True,
        **kwargs
    )
    val_dataset = val_dataset.map(
        lambda ele: tokenize_inputs(config, tokenizer, ele), 
        batched=True,
        **kwargs
    )
    train_dataset = train_dataset.with_format("torch")
    val_dataset = val_dataset.with_format("torch")

    return train_dataset, val_dataset
