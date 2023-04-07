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
    newline_tokens = tokenizer("\n", return_tensors="pt")["input_ids"][0, 1:]

    out = {"labels": [], "attention_mask": []}
    for i, (prompt, response) in enumerate(zip(examples["prompt"], examples["response"])):
        input_tokens = tokenizer(prompt, truncation=True, max_length=max_length // 2, return_tensors="pt")["input_ids"].squeeze()
        input_len = len(input_tokens)

        # plus one since we remove bos from response
        # but we subtract one since we want to add eos token
        remaining_tokens = max_length - input_len - len(newline_tokens) + 1
        # remove bos
        target_tokens = tokenizer(response, truncation=True, max_length=remaining_tokens, return_tensors="pt")["input_ids"].squeeze()[1:]

        input_ids[i, :input_len] = input_tokens
        # add newline between prompt and response
        newline_plus_inputs = input_len + len(newline_tokens)
        input_ids[i, input_len: newline_plus_inputs] = newline_tokens

        # add target tokens, remove bos
        input_ids[i, newline_plus_inputs: newline_plus_inputs + len(target_tokens)] = target_tokens
        # add eos token; ensure generation stops if inputs aren't truncated
        # we don't want long code to stop generating if truncated during training
        if newline_plus_inputs + len(target_tokens) < max_length:
            input_ids[i, newline_plus_inputs + len(target_tokens)] = tokenizer.eos_token_id

        labels = input_ids[i].clone()
        labels[: newline_plus_inputs] = -100
        labels[labels == tokenizer.pad_token_id] = -100
        # to debug this, can set all values == -100 to the pad token, then assert that tokenizer.decode(labels, skip_special_tokens=True).strip() == response

        attention_mask = input_ids[i].ne(tokenizer.pad_token_id).int()

        out["labels"].append(labels)
        out["attention_mask"].append(attention_mask)

    out["input_ids"] = input_ids

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
        dataset = load_dataset(dataset_path, split="train")

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
