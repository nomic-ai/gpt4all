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
from tqdm import tqdm

CHUNK_SIZE = 1024

def parse_args():
    parser = ArgumentParser()
    parser.add_argument("--config", type=str, default="config.yaml")
    parser.add_argument("--split", type=str, default="train")
    parser.add_argument("--k", type=int, default=5)

    return parser.parse_args()

    
def prep_index():
    args = parse_args()
    config = read_config(args.config)
    index = hnswlib.Index(space=config['index_space'], dim=config['index_dim'])
    print("loading index")
    index.load_index(config['index_path'])

    # load query dataset
    ds_path = config['dataset_path']

    # load retrieval dataset
    print("loading retrieval dataset")
    print(config["index_database"])
    if os.path.exists(config['index_database']):
        retrieval_dataset = Dataset.load_from_disk(config['index_database'])
    else:
        retrieval_dataset = load_dataset(config['index_database'], split="train")

    # vectorize queries
    query_vector_path =  f"{ds_path}_queries_embedded/{ds_path}_embedded_{args.split}"
    if not os.path.exists(query_vector_path):
        print('Embedding dataset...')
        ds = embed_texts(ds_path,
                         config['batch_size'],
                         embed_on=config['query_embedding_field'],
                         save_to_disk=False,
                         split=args.split)
        ds.save_to_disk(query_vector_path)
    else:
        print('Found cached embedding dataset!')
        ds = Dataset.load_from_disk(query_vector_path)

    #build training dataset
    train_dataset = load_dataset(ds_path, split=args.split)

    #search the index for each query
    neighbor_embs_column = []
    neighbor_ids_column = []
    for chunk_start in tqdm(range(0, len(ds), CHUNK_SIZE)):
        chunk_end = chunk_start + CHUNK_SIZE
        chunk = ds[chunk_start:chunk_end]
        query_vectors = np.array(chunk['embedding'])
        neighbor_ids, _ = index.knn_query(query_vectors, k=args.k, num_threads=-1) # neighbor ids is of shape [n_queries, n_neighbors]
        value_set = pa.array([str(e) for e in neighbor_ids.flatten()])
        neighbor_objs = retrieval_dataset._data.filter(pc.is_in(retrieval_dataset._data['id'], value_set))

        # build mapping between indices and embeddings
        neighbor_id_list = neighbor_objs['id']
        emb_list = neighbor_objs['embedding']
        idx_to_embedding = {idx.as_py(): emb_list[i] for i, idx in enumerate(neighbor_id_list)}

        neighbor_embs = []
        for cur_neighbor_ids in neighbor_ids:
            cur_embs = [idx_to_embedding[id].as_py() for id in cur_neighbor_ids]
            neighbor_embs.append(cur_embs)

        neighbor_embs_column.extend(neighbor_embs)
        neighbor_ids_column.extend(neighbor_ids)

    print("adding neighbor ids")
    train_dataset = train_dataset.add_column('neighbor_ids', neighbor_ids_column)
    print("adding neighbor embeddings")
    train_dataset = train_dataset.add_column('neighbor_embeddings', neighbor_embs_column)

    supplemented_dataset_path = f"{ds_path}_supplemented_{args.split}/"
    train_dataset.save_to_disk(supplemented_dataset_path)


if __name__ == "__main__":
    prep_index()