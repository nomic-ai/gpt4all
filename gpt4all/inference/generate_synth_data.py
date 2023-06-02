from argparse import ArgumentParser
from datasets import load_dataset, Dataset
import torch
from torch.utils.data import DataLoader, DistributedSampler
from accelerate.utils import set_seed
from transformers import AutoTokenizer, AutoModelForCausalLM, DefaultDataCollator 
from gpt4all.utils.read import read_config
from gpt4all.utils.distributed_utils import rank0_print, main_process_first
from tqdm import tqdm
import pyarrow.compute as pc
import pyarrow as pa
import torch.distributed as dist


PROMPT = "Write a question answer pair based on the following context. If the context isn't specific enough, ignore and return 'No question answer pair`. Context : {}\n"


def prepare_data(config, tokenizer, num_processes, local_rank):
    dataset = load_dataset(config["dataset_path"], split="train")
    dataset = dataset.remove_columns("embedding")

    shuffled = dataset.shuffle(seed=config["seed"])
    indices = shuffled[:config["max_generations"]]["id"]

    table = dataset.data
    mask = pc.is_in(table["id"], value_set=pa.array(indices, pa.int32()))
    filtered_table = table.filter(mask)

    # convert from pyarrow to Dataset
    orig_dataset = Dataset.from_dict(filtered_table.to_pydict())

    dataset = orig_dataset.map(lambda ele: {"prompted_text": [PROMPT.format(context) for context in ele["text"]]},
                          batched=True,
                          num_proc=config["num_proc"] if "num_proc" in config else None)

    dataset = dataset.map(lambda ele: {"prompt_len": [len(prompt) for prompt in ele["prompted_text"]]}, batched=True,
                          num_proc=config["num_proc"] if "num_proc" in config else None)

    dataset = dataset.sort("prompt_len")
    dataset = dataset.map(lambda ele: tokenizer(ele["prompted_text"], return_tensors="pt", padding="longest", truncation=True,
                                                max_length=tokenizer.model_max_length - config["max_new_tokens"]), batched=True,
                                                batch_size=num_processes * config["batch_size"],
                         )

    columns_to_keep = ["id", "input_ids", "attention_mask"]
    col_names_to_rm = [col for col in dataset.column_names if col not in columns_to_keep]

    dataset = dataset.remove_columns(col_names_to_rm)

    sampler = DistributedSampler(
        dataset,
        shuffle=False,
        drop_last=True,
        num_replicas=num_processes,
        rank=local_rank,
    )

    dataloader = DataLoader(
            dataset,
            batch_size=config["batch_size"],
            collate_fn=DefaultDataCollator(),
            sampler=sampler,
            drop_last=True
        )

    return dataloader, orig_dataset

def generate_data(config):
    set_seed(config["seed"])

    rank0_print(f"World size: {dist.get_world_size()}")

    tokenizer = AutoTokenizer.from_pretrained(config["tokenizer_name"])
    # since we're doing generation, pad left for autoregressive generation
    tokenizer.padding_side = "left"
    if tokenizer.pad_token is None:
        tokenizer.pad_token = tokenizer.eos_token

    num_processes = dist.get_world_size()
    local_rank = dist.get_rank()

    dataloader, dataset = prepare_data(config, tokenizer, num_processes, local_rank)

    dist.barrier()
    print(dataset[:10]["id"])
        
    model = AutoModelForCausalLM.from_pretrained(config["model_name"],
                                                       revision=config["revision"] if "revision" in config else None,
                                                       use_cache=True,
                                                       torch_dtype=torch.bfloat16,)
    model.to(f"cuda:{local_rank}")

    synth_data = []
    valid = []
    ids = []
    total_valid = 0

    with torch.no_grad():
        for i, batch in enumerate(tqdm(dataloader, disable=local_rank != 0)):
            # keep this simple for now, can add temperature and other sampling techniques later
            generated = model.generate(batch["input_ids"].to(model.device),
                                       attention_mask=batch["attention_mask"].to(model.device),
                                       max_new_tokens=config["max_new_tokens"])

            decoded = tokenizer.batch_decode(generated, skip_special_tokens=True)
            num_valid = ["\nQuestion:" in t and "\nAnswer:" in t for t in decoded]
            rank0_print(f"Num valid: {sum(num_valid)/ len(num_valid):.2f}")
            total_valid += sum(num_valid)

            synth_data.extend(decoded)
            valid.extend(num_valid)
            ids.extend(batch["id"].tolist())

            if i > 0 and i % config["save_every"] == 0:
                table = dataset.data.table
                mask = pc.is_in(table["id"], value_set=pa.array(ids, pa.int32()))
                filtered_table = table.filter(mask)

                chunk_table = pa.Table.from_pydict({"id": ids, "generated": synth_data, "valid": valid})
                joined = filtered_table.join(chunk_table, "id")
                curr_dataset = Dataset.from_dict(joined.to_pydict())
                curr_dataset.save_to_disk(f'{config["output_path"]}/chunk_{i}_rank_{local_rank}')

    table = dataset.data.table
    mask = pc.is_in(table["id"], value_set=pa.array(ids, pa.int32()))
    filtered_table = table.filter(mask)

    chunk_table = pa.Table.from_pydict({"id": ids, "generated": synth_data, "valid": valid})
    joined = filtered_table.join(chunk_table, "id")
    full_dataset = Dataset.from_dict(joined.to_pydict())
    full_dataset.save_to_disk(f'{config["output_path"]}_{config["max_generations"]}_rank_{local_rank}')

    rank0_print(f"Total valid: {total_valid}/{config['max_generations']}")

    
def main():
    dist.init_process_group("nccl")
    parser = ArgumentParser()
    parser.add_argument("--config", type=str, default="config.yaml")

    args = parser.parse_args()
    config = read_config(args.config)

    generate_data(config)


if __name__ == "__main__":
    # parse arguments by reading in a config
    main()