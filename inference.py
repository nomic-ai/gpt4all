from transformers import AutoModelForCausalLM, AutoTokenizer
import torch
import torch.nn as nn
from argparse import ArgumentParser
from read import read_config
from accelerate.utils import  set_seed
from data import load_data_for_inference
from tqdm import tqdm
from datasets import concatenate_datasets, Dataset
import torch.distributed as dist
from transformers.trainer_pt_utils import ShardSampler, distributed_concat, nested_numpify
from transformers import DefaultDataCollator
from torch.utils.data import DataLoader
import numpy as np


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

    train_sampler = ShardSampler(train_dataset, config["batch_size"], num_processes=num_processes, process_index=local_rank)
    train_dataloader = DataLoader(
        train_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
        sampler=train_sampler
    )

    val_sampler = ShardSampler(val_dataset, config["batch_size"], num_processes=num_processes, process_index=local_rank)
    val_dataloader = DataLoader(
        val_dataset,
        collate_fn=DefaultDataCollator(),
        batch_size=config["batch_size"],
        sampler=val_sampler
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

        dist.barrier()
        gathered_train = nested_numpify(distributed_concat(train_outputs))
        gathered_train["index"] = np.concatenate(gathered_train["index"])
        gathered_train["loss"] = np.concatenate(gathered_train["loss"])
        gathered_train["embeddings"] = np.concatenate(gathered_train["embeddings"])

        df_train = Dataset.from_dict(gathered_train)
        df_train = df_train.sort("index")

        train_dataset = train_dataset.add_column("embeddings", df_train["embeddings"])
        train_dataset = train_dataset.add_column("loss", df_train["loss"])
        train_dataset = train_dataset.add_column("is_train", [True] * len(train_dataset))

        val_outputs = {"loss": [], "embeddings": [], "index": []}
        for batch in tqdm(val_dataloader, disable=local_rank != 0):
            batch["input_ids"] = batch["input_ids"].to(f"cuda:{local_rank}")
            batch["labels"] = batch["labels"].to(f"cuda:{local_rank}")
            outputs = model(input_ids=batch["input_ids"], labels=batch["labels"])
            loss = calc_cross_entropy_no_reduction(outputs.logits, batch["labels"])
            val_outputs["loss"].extend(loss)

            logits = outputs.logits
            batch_size = batch["input_ids"].shape[0]
            sequence_lengths = []
            # since we use mutiturn with multiple <|endoftext|>, we need to find the place where 
            # <|endoftext|> is repeated
            for item in batch["input_ids"]:
                indices = torch.where(item == tokenizer.pad_token_id)[0]
                found = False
                for index in indices:
                    if torch.all(item[index:] == tokenizer.pad_token_id):
                        sequence_lengths.append(index)
                        found = True
                        break

                # no match found
                if not found:
                    sequence_lengths.append(len(item) - 1)

            sequence_lengths = torch.tensor(sequence_lengths)
            pooled_logits = logits[torch.arange(batch_size, device=logits.device), sequence_lengths]

            val_outputs["embeddings"].append(pooled_logits)
            val_outputs["index"].extend(batch["index"].to(model.device))

            torch.cuda.empty_cache()

        dist.barrier() 
        gathered_val = nested_numpify(distributed_concat(val_outputs))

        gathered_val["index"] = np.concatenate(gathered_val["index"])
        gathered_val["loss"] = np.concatenate(gathered_val["loss"])
        gathered_val["embeddings"] = np.concatenate(gathered_val["embeddings"])

        df_val = Dataset.from_dict(gathered_val)
        df_val = df_val.sort("index")

        val_dataset = val_dataset.add_column("embeddings", df_val["embeddings"])
        val_dataset = val_dataset.add_column("loss", df_val["loss"])
        val_dataset = val_dataset.add_column("is_train", [False] * len(val_dataset))

    df = concatenate_datasets([train_dataset, val_dataset])
    if local_rank == 0:
        df.to_json("epoch_1_checkpoint.jsonl", lines=True, orient="records", num_proc=64)

    
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
    
