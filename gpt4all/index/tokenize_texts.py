from datasets import load_dataset
from argparse import ArgumentParser
from gpt4all.index.embed import Embedder


def parse_args():
    parser = ArgumentParser()
    # fmt: off
    parser.add_argument("--tokenized_save_path", type=str, default="tokenized")
    parser.add_argument("--ds_name", type=str, default="wikipedia")
    parser.add_argument("--ds_version", type=str, default="20220301.simple")
    parser.add_argument("--sbert_model", type=str, default="sentence-transformers/all-MiniLM-L6-v2")
    # fmt: on

    return parser.parse_args()


def tokenize_texts(examples, embedder):
    split_data = {k: [] for k in examples.keys()}
    split_data["tokenized_chunk"] = []
    split_data["tokenized_attn_mask"] = []

    keys = [k for k in examples.keys() if k != "text"]
    for i, text in enumerate(examples["text"]):
        tokenized_text = embedder.chunk_text(text)
        # do we want to add sep/cls tokens to beginning and end?
        decoded_text = embedder.tokenizer.batch_decode(
            sequences=tokenized_text["input_ids"]
        )

        num_splits = len(tokenized_text["input_ids"])
        split_data["id"].extend(
            [f"{examples['id'][i]}_split_{j}" for j in range(num_splits)]
        )

        for col in keys:
            if col != "id":
                split_data[col].extend(
                    [examples[col][i]] * len(tokenized_text["input_ids"])
                )

        split_data["text"].extend(decoded_text)
        split_data["tokenized_chunk"].extend(tokenized_text["input_ids"])
        split_data["tokenized_attn_mask"].extend(tokenized_text["attention_mask"])

    return split_data


def chunk_dataset(
    ds_name="wikipedia",
    version="20220301.simple",
    sbert_model="sentence-transformers/all-MiniLM-L6-v2",
    save_path="tokenized",
):
    dataset = load_dataset(ds_name, version, split="train")
    print(len(dataset))
    embedder = Embedder(sbert_model)
    dataset = dataset.map(
        lambda x: tokenize_texts(x, embedder), batched=True, num_proc=64
    )

    dataset.save_to_disk(save_path)


if __name__ == "__main__":
    args = parse_args()
    chunked_dataset = chunk_dataset(
        ds_name=args.ds_name,
        version=args.ds_version,
        sbert_model=args.sbert_model,
        save_path=args.tokenized_save_path,
    )
