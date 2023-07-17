from tqdm import tqdm
import evaluate
import torch
from torch.utils.data import DataLoader
from transformers import DefaultDataCollator, AutoTokenizer, AutoModelForCausalLM
from argparse import ArgumentParser
from datasets import load_dataset


def parse_args():
    parser = ArgumentParser()
    parser.add_argument("--model", type=str, default="gpt2")
    parser.add_argument("--dataset", type=str, default="cnn_dailymail")
    parser.add_argument("--version", type=str, default=None)
    parser.add_argument("--batch_size", type=int, default=32)
    parser.add_argument("--max_new_tokens", type=int, default=75)

    return parser.parse_args()

   
def prefix(row, column_name, prefix):
    row[column_name] = prefix + row[column_name] + "\n"
    return row

    
def process_dataset(dataset_name, dataset, tokenizer, batch_size):
    if dataset_name == "gigaword":
        dataset = dataset.map(lambda x: prefix(x, "document", "Generate a short short summary: "))
        dataset = dataset.rename_column("document", "inputs")
        labels = dataset["summary"]
    elif dataset_name == "cnn_dailymail":
        dataset = dataset.map(lambda x: prefix(x, "article", "Summarize the following text: "))
        dataset = dataset.rename_column("article", "inputs")
        labels = dataset["highlights"]
    elif dataset_name == "xsum":
        dataset = dataset.map(lambda x: prefix(x, "document", "Summarize the following text: "))
        dataset = dataset.rename_column("document", "inputs")
        labels = dataset["summary"]
    else:
        raise ValueError("Dataset not supported")

        
    dataset = dataset.map(lambda x: tokenizer(x["inputs"], padding="longest", truncation=True, return_tensors="pt"),
                          batched=True, batch_size=batch_size)

    dataset = dataset.map(lambda x: {"lengths": [len(tokens) for tokens in x["input_ids"]]}, batched=True)

    print(len(dataset))
    dataset = dataset.filter(lambda x: x["lengths"] <= 2048)
    print(len(dataset))

    columns_to_keep = ["input_ids", "attention_mask", "labels"]
    columns_to_remove = [col for col in dataset.column_names if col not in columns_to_keep]
    dataset = dataset.remove_columns(columns_to_remove)

    dataloader = DataLoader(dataset, batch_size=batch_size, collate_fn=DefaultDataCollator())

    return dataloader, labels



def load_model(model_name):
    tokenizer = AutoTokenizer.from_pretrained(model_name, use_fast=False)
    tokenizer.pad_token = tokenizer.eos_token
    tokenizer.padding_side = "left"

    model = AutoModelForCausalLM.from_pretrained(model_name, use_cache=True, torch_dtype=torch.bfloat16)

    return model, tokenizer


def calculate_rouge(model, labels, dataloader, max_num_tokens, device="cuda"):
    rouge_score = evaluate.load("rouge")

    model.to(device)

    sliced_seq = []
    for batch in tqdm(dataloader):
        batch = {key: value.to(model.device) for key, value in batch.items()}
        outputs = model.generate(
            **batch,
            max_new_tokens=max_num_tokens,
        )
        decoded = tokenizer.batch_decode(outputs, skip_special_tokens=True)

        prompted_inputs = batch["input_ids"]
        for j, seq in enumerate(decoded):
            # get generated sequence without the prompt
            sliced_seq.append(seq[len(tokenizer.decode(prompted_inputs[j], skip_special_tokens=True)) :])

    score = rouge_score.compute(predictions=sliced_seq, references=labels)
    print(score)


if __name__ == "__main__":
    args = parse_args()
    
    dataset = load_dataset(args.dataset, args.version, split="test")

    model, tokenizer = load_model(args.model)

    dataloader, labels = process_dataset(args.dataset, dataset, tokenizer, args.batch_size)

    calculate_rouge(model, labels, dataloader, args.max_new_tokens)