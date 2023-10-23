#!/usr/bin/env python3
from transformers import AutoModelForCausalLM, AutoTokenizer
import torch
import torch.nn as nn
from argparse import ArgumentParser
from read import read_config
from accelerate.utils import  set_seed
from data import load_data_for_inference
from tqdm import tqdm
from datasets import  Dataset
import torch.distributed as dist
from transformers.trainer_pt_utils import  nested_numpify
from transformers import DefaultDataCollator
from torch.utils.data import DataLoader, DistributedSampler
import numpy as np
import pyarrow as pa
from pyarrow import compute as pc


def calc_cross_entropy_no_reduction(lm_logits, labels):
    # calculate cross entropy across batch dim
    shift_logits = lm_logits[..., :-1, :].contiguous()
    shift_labels = labels[..., 1:].contiguous()
    # Flatten the tokens
    loss_fct = nn.CrossEntropyLoss(reduction='none')
    loss = loss_fct(shift_logits.permute(0, 2, 1), shift_labels).mean(dim=1)

    return loss


def rank0_print(msg):
    if dist.get_rank() == 0:
        print(msg)
        

def inference(config):
    set_seed(config['seed'])

    rank0_print(f"World size: {dist.get_world_size()}")

    tokenizer = AutoTokenizer.from_pretrained(config['tokenizer_name'], model_max_length=config['max_length'])
    # llama has no pad token, set it to new token
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token

        
    train_dataset, val_dataset = load_data_for_inference(config, tokenizer) 

    num_processes = dist.get_world_size()
    local_rank = dist.get_rank()

    train_sampler = DistributedSampler(train_dataset, shuffle=False, drop_last=True, num_replicas=num_processes, rank=local_rank)
    train_dataloader = DataLoader(
        train_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
        sampler=train_sampler,
        drop_last=True
    )

    val_sampler = DistributedSampler(val_dataset, shuffle=False, drop_last=True, num_replicas=num_processes, rank=local_rank)
    val_dataloader = DataLoader(
        val_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
        sampler=val_sampler,
        drop_last=True
    )


    model = AutoModelForCausalLM.from_pretrained(config["model_name"], 
                                                    trust_remote_code=True,
                                                    torch_dtype=torch.bfloat16,
                                                    ) 
    model.to(f"cuda:{local_rank}")

    with torch.no_grad():
        train_outputs = {"loss": [], "embeddings": [], "index": []}
        for batch in tqdm(train_dataloader, disable=local_rank != 0):
            batch["input_ids"] = batch["input_ids"].to(f"cuda:{local_rank}")
            batch["labels"] = batch["labels"].to(f"cuda:{local_rank}")
            outputs = model(input_ids=batch["input_ids"], labels=batch["labels"], output_hidden_states=True)
            loss = calc_cross_entropy_no_reduction(outputs.logits, batch["labels"])
            train_outputs["loss"].extend(loss)

            embeddings = outputs.hidden_states[-1]
            batch_size = batch["input_ids"].shape[0]
            sequence_lengths = []
            # since we use mutiturn with multiple <|endoftext|>, we need to find the place where 
            # <|endoftext|> is repeated
            for item in batch["input_ids"]:
                indices = torch.where(item == tokenizer.pad_token_id)[0]
                found = False
                for index in indices:
                    # case where sequence is less than max length
                    if torch.all(item[index:] == tokenizer.pad_token_id):
                        sequence_lengths.append(index)
                        found = True
                        break
                # case where sequence is >= max length
                if not found:
                    sequence_lengths.append(len(item) - 1)

            sequence_lengths = torch.tensor(sequence_lengths)
            pooled_logits = embeddings[torch.arange(batch_size, device=embeddings.device), sequence_lengths]

            train_outputs["embeddings"].append(pooled_logits)
            train_outputs["index"].extend(batch["index"].to(model.device))

            torch.cuda.empty_cache()

        train_outputs = nested_numpify(train_outputs)
        # stack since they're 0-dim arrays
        train_outputs["index"] = np.stack(train_outputs["index"])
        train_outputs["loss"] = np.stack(train_outputs["loss"])
        train_outputs["embeddings"] = np.concatenate(train_outputs["embeddings"])

        df_train = Dataset.from_dict(train_outputs)
        curr_idx = df_train["index"]

        # compute mask in pyarrow since it's super fast
        # ty @bmschmidt for showing me this!
        table = train_dataset.data
        mask = pc.is_in(table['index'], value_set=pa.array(curr_idx, pa.int32()))
        filtered_table = table.filter(mask)
        # convert from pyarrow to Dataset
        filtered_train = Dataset.from_dict(filtered_table.to_pydict())

        filtered_train = filtered_train.add_column("embeddings", df_train["embeddings"])
        filtered_train = filtered_train.add_column("loss", df_train["loss"])
        filtered_train = filtered_train.add_column("is_train", [True] * len(filtered_train))

        filtered_train.to_json(f"inference/epoch_2_embeddings_train_shard_{local_rank}.jsonl", lines=True, orient="records", num_proc=64)

        val_outputs = {"loss": [], "embeddings": [], "index": []}
        for batch in tqdm(val_dataloader, disable=local_rank != 0):
            batch["input_ids"] = batch["input_ids"].to(f"cuda:{local_rank}")
            batch["labels"] = batch["labels"].to(f"cuda:{local_rank}")
            outputs = model(input_ids=batch["input_ids"], labels=batch["labels"], output_hidden_states=True)
            loss = calc_cross_entropy_no_reduction(outputs.logits, batch["labels"])
            val_outputs["loss"].extend(loss)

            embeddings = outputs.hidden_states[-1]
            batch_size = batch["input_ids"].shape[0]
            sequence_lengths = []
            # since we use mutiturn with multiple <|endoftext|>, we need to find the place where 
            # <|endoftext|> is repeated
            for item in batch["input_ids"]:
                indices = torch.where(item == tokenizer.pad_token_id)[0]
                found = False
                for index in indices:
                    # case where sequence is less than max length
                    if torch.all(item[index:] == tokenizer.pad_token_id):
                        sequence_lengths.append(index)
                        found = True
                        break
                # case where sequence is >= max length
                if not found:
                    sequence_lengths.append(len(item) - 1)

            sequence_lengths = torch.tensor(sequence_lengths)
            pooled_logits = embeddings[torch.arange(batch_size, device=embeddings.device), sequence_lengths]

            val_outputs["embeddings"].append(pooled_logits)
            val_outputs["index"].extend(batch["index"].to(model.device))

            torch.cuda.empty_cache()

        val_outputs = nested_numpify(val_outputs)
        val_outputs["index"] = np.stack(val_outputs["index"])
        val_outputs["loss"] = np.stack(val_outputs["loss"])
        val_outputs["embeddings"] = np.concatenate(val_outputs["embeddings"])

        df_val = Dataset.from_dict(val_outputs)
        curr_idx = df_val["index"]

        # compute mask in pyarrow since it's super fast
        # ty @bmschmidt for showing me this!
        table = val_dataset.data
        mask = pc.is_in(table['index'], value_set=pa.array(curr_idx, pa.int32()))
        filtered_table = table.filter(mask)
        # convert from pyarrow to Dataset
        filtered_val = Dataset.from_dict(filtered_table.to_pydict())
        filtered_val = filtered_val.add_column("embeddings", df_val["embeddings"])
        filtered_val = filtered_val.add_column("loss", df_val["loss"])
        filtered_val = filtered_val.add_column("is_train", [False] * len(filtered_val))

        filtered_val.to_json(f"inference/epoch_2_embeddings_val_shard_{local_rank}.jsonl", lines=True, orient="records", num_proc=64)
    

def main():
    dist.init_process_group("nccl")
    parser = ArgumentParser()
    parser.add_argument("--config", type=str, default="config.yaml")

    args = parser.parse_args()
    config = read_config(args.config)

    inference(config)


if __name__ == "__main__":
    # parse arguments by reading in a config
    main()
    
