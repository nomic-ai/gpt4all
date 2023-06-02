import glob
from argparse import ArgumentParser
from datasets import Dataset, load_from_disk, concatenate_datasets


PROMPT = "Write a question answer pair based on the following context. If the context isn't specific enough, ignore and return 'No question answer pair`. Context : {}\n"

def load_synth_data(data_dir):
    files = glob.glob(data_dir + "/*")

    ds = concatenate_datasets([load_from_disk(f) for f in files])
    table = ds.data.table

    filtered = table.filter(table["valid"])
    ds = Dataset.from_dict(filtered.to_pydict())
    return ds 

    
def remove_prompt(examples):
    outputs = {"text": [], "generated": [], "question": [], "answer": []}
    for context, generated in zip(examples["text"], examples["generated"]):
        prompt_w_ctx = PROMPT.format(context)
        gen_wo_ctx = generated[len(prompt_w_ctx):]

        assert prompt_w_ctx not in gen_wo_ctx

        question = gen_wo_ctx.split("Answer:")[0].replace("Question:", "").strip()
        answer = gen_wo_ctx.split("Answer:")[1].strip()

        outputs["text"].append(context)
        outputs["generated"].append(gen_wo_ctx)
        outputs["question"].append(question)
        outputs["answer"].append(answer)

    return outputs

        


def combine_synth_data(data_dir):
    ds = load_synth_data(data_dir)

    ds = ds.map(lambda ele: remove_prompt(ele), batched=True, num_proc=64)

    ds.save_to_disk(f"synth_data_combined_{len(ds)/1000:.0f}")



if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--dataset_dir", type=str, required=True)

    args = parser.parse_args()

    combine_synth_data(args.dataset_dir)

    