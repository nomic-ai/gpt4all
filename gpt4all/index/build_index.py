import os
from datasets import Dataset, concatenate_datasets
import glob
from argparse import ArgumentParser
import hnswlib
import pyarrow as pa
import pyarrow.compute as pc
from tqdm import tqdm


def parse_args():
    parser = ArgumentParser()
    parser.add_argument("--ds_path", type=str, required=True)
    parser.add_argument("--embed_folder", type=str, required=True)
    parser.add_argument("--index_path", type=str, default="wiki-index")

    return parser.parse_args()


def concat_embedded(folder):
    files = glob.glob(f"{folder}/*")

    all_embeddings = []
    for file in files:
        all_embeddings.append(Dataset.load_from_disk(file))

    all_embeddings = concatenate_datasets(all_embeddings)

    return all_embeddings


def join(original_ds, embedded_ds):
    embedded_ds = embedded_ds.add_column("index", range(len(embedded_ds)))
    embed_table = embedded_ds.data.table

    seen = set()
    indices = []
    for i, id in enumerate(original_ds["id"]):
        if id not in seen:
            indices.append(i)
            seen.add(id)

    mask = pc.is_in(embed_table["index"], value_set=pa.array(indices, pa.int32()))
    filtered_table = embed_table.filter(mask)

    import pdb; pdb.set_trace()
    # sort to make sure we're adding in right order
    filtered_table = filtered_table.sort_by("id")

    original_table = original_ds.data.table
    original_table = original_table.sort_by("id")

    original_table = original_table.append_column(
        "embedding", filtered_table["embedding"]
    )
    # there's definitely a better way to do this but
    # Dataset(original_table) throws `KeyError: 'embedding'`
    joined = Dataset.from_dict(original_table.to_pydict())

    return joined


def build_index(orig_path, embed_folder_path, index_path):
    if not os.path.exists(orig_path + "_embedded_with_text"):
        # TODO: this doesn't work for large datasets!
        # just convert to pandas and do this manually
        ds = Dataset.load_from_disk(orig_path)
        embed_ds = concat_embedded(embed_folder_path)
        print("Concatenated embeddings")
        print(f"Length: {len(ds)}")
        print(f"Length: {len(embed_ds)}")
        ds = join(ds, embed_ds)
        ds = ds.add_column("index", range(len(ds)))
        print("Saving to disk")
        ds.save_to_disk(f"{orig_path}_embedded_with_text")
    else:
        ds = Dataset.load_from_disk(orig_path + "_embedded_with_text")

    print(f"Length of ds: {len(ds)}")

    print("Building index")
    index = hnswlib.Index(space="cosine", dim=384)
    # not sure what we should set M and ef_construction to
    index.init_index(max_elements=len(ds), M=64, ef_construction=200)
    print("Adding items")
    chunk_size = 50_000
    num_chunks = len(ds) // chunk_size
    progbar = tqdm(total=num_chunks)
    start = 0
    while start < len(ds):
        chunk = ds[start:start + chunk_size]
        index.add_items(chunk["embedding"], chunk["index"], num_threads=64)
        progbar.update(1)
        start += chunk_size

    print("Saving index")
    index.save_index(index_path + ".bin")


if __name__ == "__main__":
    args = parse_args()
    build_index(args.ds_path, args.embed_folder, args.index_path)
