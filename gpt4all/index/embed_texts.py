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


def embed_texts(ds_path, batch_size):
    rank0_print(f"World size: {dist.get_world_size()}")

    dataset = Dataset.load_from_disk(ds_path)
    rank0_print(f"Dataset size: {len(dataset)}")
    dataset = dataset.remove_columns(["url", "title", "text"])
    dataset = dataset.with_format("torch")

    num_processes = dist.get_world_size()
    local_rank = dist.get_rank()

    model = Embedder()

    collator = PadCollateInputs(model.tokenizer)

    sampler = DistributedSampler(
        dataset,
        shuffle=False,
        drop_last=False,
        num_replicas=num_processes,
        rank=local_rank,
    )
    dataloader = DataLoader(
        dataset,
        collate_fn=collator,
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
    ds.save_to_disk(f"embedded/{ds_path}_embedded_rank_{local_rank}")


def main():
    dist.init_process_group("nccl")
    parser = ArgumentParser()
    parser.add_argument("--ds_path", type=str, default="tokenized")
    parser.add_argument("--batch_size", type=int, default=1)

    args = parser.parse_args()

    embed_texts(args.ds_path, args.batch_size)


if __name__ == "__main__":
    # parse arguments by reading in a config
    main()
