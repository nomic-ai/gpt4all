from datasets import load_dataset

from transformers import AutoTokenizer, AutoModelForCausalLM

tokenizer = AutoTokenizer.from_pretrained("gpt2")
model = AutoModelForCausalLM.from_pretrained("gpt2")
tokenizer.pad_token = tokenizer.eos_token
tokenizer.padding_side = "left"

import ast
from tqdm import tqdm
from torch.utils.data import DataLoader
from transformers import DefaultDataCollator
from sacrebleu.metrics import BLEU

bleu = BLEU()


# Testing with gpt2
from datasets import load_dataset
from transformers import AutoTokenizer, AutoModelForCausalLM

tokenizer = AutoTokenizer.from_pretrained("gpt2")
model = AutoModelForCausalLM.from_pretrained("gpt2")
tokenizer.pad_token = tokenizer.eos_token
tokenizer.padding_side = "left"


dataset = load_dataset(
    "lighteval/sacrebleu_manual",
    "wmt18_test-ts_de-en",
    # Use ^ that have test in them
    split="test[:50]",
)


refs = []


def prefix(x):
    y = ast.literal_eval(x["translation"])
    x["translation"] = (
        "Translate the following from "
        + list(y)[0]
        + " to "
        + list(y)[1]
        + ": "
        + y[list(y)[0]]
    )
    refs.append(y[list(y)[1]])
    return x


dataset = dataset.map(prefix)
inputs = dataset.map(
    lambda x: tokenizer(
        x["translation"],
        padding="longest",
        truncation=True,
        return_tensors="pt",
    ),
    batched=True,
    batch_size=10,
)
inputs.set_format(
    type="torch",
    columns=[
        "input_ids",
        "attention_mask",
    ],
)
ids = inputs["input_ids"]
length = max([len(i) for i in ids]) + 1
masks = inputs["attention_mask"]
dataloader = DataLoader(inputs, batch_size=10, collate_fn=DefaultDataCollator())


sliced_seq = []
for batch in tqdm(dataloader):
    batch = {key: value.to(model.device) for key, value in batch.items()}
    outputs = model.generate(
        **batch,
        max_new_tokens=100,
    )
    decoded = tokenizer.batch_decode(outputs, skip_special_tokens=True)
    jd = batch["input_ids"]
    for j, seq in enumerate(decoded):
        sliced_seq.append(seq[len(tokenizer.decode(jd[j], skip_special_tokens=True)) :])
score = bleu.corpus_score(hypotheses=sliced_seq, references=refs)
print(score)
