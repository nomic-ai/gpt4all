import os
import torch.distributed as dist
from argparse import ArgumentParser
from datasets import Dataset
from gpt4all.index.embed import Embedder
from gpt4all.utils.distributed_utils import rank0_print
from torch.utils.data import DataLoader, DistributedSampler
from transformers.trainer_pt_utils import nested_numpify
from transformers import BatchEncoding
from tqdm import tqdm
import numpy as np
import torch
from datasets import load_dataset


# this isn't used but keeping in case we need it in the future
# collate and pad inputs to the right shape
class PadCollateInputs:
    def __init__(self, tokenizer):
        self.tokenizer = tokenizer

    def __call__(self, batch):
        mapped_inputs = {"input_ids": [], "attention_mask": []}
        mapped_inputs["input_ids"] = [b["tokenized_chunk"] for b in batch]
        mapped_inputs["attention_mask"] = [b["tokenized_attn_mask"] for b in batch]
        encoding = BatchEncoding(mapped_inputs)

        padded_inputs = self.tokenizer.pad(
            encoding, padding="max_length", return_tensors="pt"
        )
        padded_inputs["id"] = [b["id"] for b in batch]

        return padded_inputs


def embed_texts(ds_path, batch_size, embed_on='text', save_to_disk=False, split='train'):
    world_size = dist.get_world_size() if dist.is_initialized() else 1
    rank0_print(f"World size: {world_size}")

    if os.path.exists(ds_path):
        dataset = Dataset.load_from_disk(ds_path)
    else:
        dataset = load_dataset(ds_path, split=split)
    rank0_print(f"Dataset size: {len(dataset)}")

    model = Embedder()

    if "input_ids" not in dataset.column_names:
        dataset = dataset.map(lambda x: model.tokenize(x[embed_on]), batched=True, num_proc=64)


    columns_to_keep = ["id", "input_ids", "attention_mask"]
    to_remove = [e for e in dataset.column_names if not e in columns_to_keep]
    dataset = dataset.remove_columns(to_remove)

    dataset = dataset.with_format("torch")

    num_processes = dist.get_world_size() if dist.is_initialized() else 1
    local_rank = dist.get_rank() if dist.is_initialized() else 0 


    if num_processes > 1: 
        sampler = DistributedSampler(
            dataset,
            shuffle=False,
            drop_last=False,
            num_replicas=num_processes,
            rank=local_rank,
        )
    else:
        sampler = None

    dataloader = DataLoader(
        dataset,
        batch_size=batch_size,
        sampler=sampler,
        drop_last=False,
    )

    model.to(f"cuda:{local_rank}")
    with torch.no_grad():
        embedded_outputs = {"id": [], "embedding": []}
        for batch in tqdm(dataloader, disable=local_rank != 0):
            batch["input_ids"] = batch["input_ids"].to(f"cuda:{local_rank}")
            batch["attention_mask"] = batch["attention_mask"].to(f"cuda:{local_rank}")
            outputs = model(batch)
            embedded_outputs["id"].extend(batch["id"])
            embedded_outputs["embedding"].extend(outputs["embedding"])

        embedded_outputs["embedding"] = nested_numpify(embedded_outputs["embedding"])
        embedded_outputs["id"] = np.stack(embedded_outputs["id"])
        embedded_outputs["embedding"] = np.stack(embedded_outputs["embedding"])

    ds = Dataset.from_dict(embedded_outputs)

    # feeling lazy, don't want to wait for all_gather to finish
    # will load and concat in a single process in another script
    if save_to_disk:
        ds.save_to_disk(f"{ds_path}_embedded/{ds_path}_embedded_rank_{local_rank}")
    return ds


def main():
    dist.init_process_group("nccl")
    parser = ArgumentParser()
    parser.add_argument("--ds_path", type=str, default="tokenized")
    parser.add_argument("--batch_size", type=int, default=1)

    args = parser.parse_args()

    embed_texts(args.ds_path, args.batch_size, save_to_disk=True)


if __name__ == "__main__":
    # parse arguments by reading in a config
    main()
