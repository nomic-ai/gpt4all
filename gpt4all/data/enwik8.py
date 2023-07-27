import numpy as np
import torch
from datasets import load_dataset
from torch.utils.data import Dataset, DataLoader
from transformers import DefaultDataCollator



class EnWik8Dataset(Dataset):
    def __init__(self, data, seq_len):
        # pyarrow chunked array
        self.data = torch.from_numpy(data)
        self.seq_len = seq_len

    def __getitem__(self, index):
        full_seq = self.data[index].long()
        return full_seq.cuda()

    def __len__(self):
        return len(self.data)
    

def load_enwik8_dataloader(config, tokenizer):
    ds = load_dataset(config["dataset_path"], split="train")

    ds = ds.train_test_split(test_size=0.05, seed=config['seed'])

    train_ds, val_ds = ds["train"], ds["test"]

    keep_cols = ["input_ids"]

    train_ds = train_ds.map(lambda x: {"len": [len(t) for t in x["text"]]}, batched=True)
    train_ds = train_ds.sort("len")
    train_ds = train_ds.map(lambda x: tokenizer(x["text"], padding="longest", truncation=True, return_tensors="pt"), 
                            batched=True,
                            batch_size=config["batch_size"])

    remove_cols = [col for col in train_ds.column_names if col not in keep_cols]
    train_ds = train_ds.remove_columns(remove_cols)

    val_ds = val_ds.map(lambda x: {"len": [len(t) for t in x["text"]]}, batched=True)
    val_ds = val_ds.sort("len")
    val_ds = val_ds.map(lambda x: tokenizer(x["text"], padding="longest", truncation=True, return_tensors="pt"),
                        batched=True,
                        batch_size=config["batch_size"])

    remove_cols = [col for col in train_ds.column_names if col not in keep_cols]
    val_ds = val_ds.remove_columns(remove_cols)

    train_dl = DataLoader(train_ds,
                          batch_size=config["batch_size"],
                          shuffle=True,
                          drop_last=True,
                          collate_fn=DefaultDataCollator())

    val_dl = DataLoader(val_ds,
                        batch_size=config["batch_size"],
                        shuffle=True,
                        drop_last=True,
                        collate_fn=DefaultDataCollator())

    return train_dl, val_dl