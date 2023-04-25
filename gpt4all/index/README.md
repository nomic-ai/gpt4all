# How to Tokenize and Embed

Split text into chunks
```
python tokenize_texts.py
```

Embbed Texts

```
torchrun --master_port=29085 --nproc-per-node 8  embed_texts.py --ds_path=tokenized --batch_size=2048
```


Combine Embeddings and Build Index
```
python build_index.py --ds_path=wiki_sample_tokenized --embed_folder=wiki_sample_embedded
```

To use the Index

```
import hnswlib

index = hnswlib.Index(space='l2', dim=384)
index.load_index(<path to index>)
```


Prep index for train

```
CUDA_VISIBLE_DEVICES=7 torchrun --master_port=29086 --nproc-per-node 1 prep_index_for_train.py --config=../../configs/train/finetune_gptjr.yaml
```
