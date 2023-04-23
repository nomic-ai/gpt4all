import os
import hnswlib
import numpy as np
import pyarrow as pa
from datasets import Dataset
import torch.distributed as dist
from datasets import load_dataset
from pyarrow import compute as pc
from argparse import ArgumentParser
from gpt4all.utils.read import read_config
from gpt4all.index.embed_texts import embed_texts

SPLIT = 'train'
CHUNK_SIZE = 1024
K = 5


if __name__ == "__main__":

    dist.init_process_group("nccl")

    parser = ArgumentParser()
    parser.add_argument("--config", type=str, default="config.yaml")

    args = parser.parse_args()

    config = read_config(args.config)

    #load index
    index = hnswlib.Index(space=config['index_space'], dim=config['index_dim'])
    index.load_index(config['index_path'])

    #load query dataset
    ds_path = config['dataset_path']

    #load retrieval dataset
    retrieval_dataset = Dataset.load_from_disk(config['index_database'])

    #vectorize queries
    query_vector_path =  f"{ds_path}_queries_embedded/{ds_path}_embedded_{SPLIT}"
    if not os.path.exists(query_vector_path):
        print('Embedding dataset...')
        ds = embed_texts(ds_path,
                         config['batch_size'],
                         embed_on=config['query_embedding_field'],
                         save_to_disk=False,
                         split=SPLIT)
        ds.save_to_disk(query_vector_path)
    else:
        print('Found cached embedding dataset!')
        ds = Dataset.load_from_disk(query_vector_path)

    #build training dataset
    train_dataset = load_dataset(ds_path, split=SPLIT)

    #search the index for each query
    neighbor_embs_column = []
    neighbor_ids_column = []
    for chunk_start in range(0, len(ds), CHUNK_SIZE):
        chunk_end = chunk_start + CHUNK_SIZE
        chunk = ds[chunk_start:chunk_end]
        query_vectors = np.array(chunk['embedding'])
        neighbor_ids, _ = index.knn_query(query_vectors, k=K, num_threads=-1)
        value_set = pa.array([str(e) for e in neighbor_ids.flatten()])
        #TODO @nussy should be id
        neighbor_objs = retrieval_dataset._data.filter(pc.is_in(retrieval_dataset._data['index'], value_set))
        neighbor_ids_column.extend(neighbor_objs['index']) #TODO @nussy should be id
        neighbor_embs_column.extend(neighbor_objs['embedding'])

    #import pdb;pdb.set_trace()
    train_dataset = train_dataset.add_column('neighbor_ids', neighbor_ids_column)
    train_dataset = train_dataset.add_column('neighbor_embeddings', neighbor_embs_column)

    supplemented_dataset_path = f"{ds_path}_supplemented_{SPLIT}/"
    train_dataset.save_to_disk(supplemented_dataset_path)
