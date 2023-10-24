#!/usr/bin/env python3
import numpy as np
from nomic import atlas
import glob
from tqdm import tqdm
from datasets import load_dataset, concatenate_datasets
from sklearn.decomposition import PCA

files = glob.glob("inference/*.jsonl")
print(files)
df = concatenate_datasets([load_dataset("json", data_files=file, split="train") for file in tqdm(files)])

print(len(df))
print(df)

df = df.map(lambda example: {"inputs": [prompt + "\n" + response for prompt, response in zip(example["prompt"], example["response"])]},
            batched=True,
            num_proc=64)

df = df.map(lambda example: {"trained_on": [int(t) for t in example["is_train"]]},
                batched=True,
                num_proc=64)
                
df = df.remove_columns("is_train")

text = df.remove_columns(["labels", "input_ids", "embeddings"])

text_df = [text[i] for i in range(len(text))]

atlas.map_text(text_df, indexed_field="inputs",
               name="CHANGE ME!",
               colorable_fields=["source", "loss", "trained_on"],
               reset_project_if_exists=True,
               )

# index is local to train/test split, regenerate
data = df.remove_columns(["labels", "input_ids", "index"])
data = data.add_column("index", list(range(len(data))))
# max embed dim is 2048 for now
# note! this is slow in pyarrow/hf datasets
embeddings = np.array(data["embeddings"])
print("embeddings shape:", embeddings.shape)
embeddings = PCA(n_components=2048).fit_transform(embeddings)

data = data.remove_columns(["embeddings"])
columns = data.to_pandas().to_dict("records") 

atlas.map_embeddings(embeddings,
                     data=columns,
                     id_field="index",
                     name="CHANGE ME!",
                     colorable_fields=["source", "loss", "trained_on"],
                     build_topic_model=True,
                     topic_label_field="inputs",
                     reset_project_if_exists=True,)
