import torch
from gpt4all.models import GPTJRForCausalLM, PythiaSeekForCausalLM
from gpt4all.data.retrieval_dataloader import load_retrieval_augmented_data
from gpt4all.train.metrics import f1_score, exact_match_score
from gpt4all.utils.read import read_config
from transformers import AutoTokenizer
from argparse import ArgumentParser
from tqdm import tqdm

parser = ArgumentParser()
parser.add_argument("--config", type=str, default="config.yaml")

args = parser.parse_args()

config = read_config(args.config)

tokenizer = AutoTokenizer.from_pretrained(config["tokenizer_name"])
if tokenizer.pad_token is None:
    tokenizer.pad_token = tokenizer.eos_token

dataloader = load_retrieval_augmented_data(config, tokenizer, split_dataset=False)


device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = PythiaSeekForCausalLM.from_pretrained(config["model_name"], use_cache=False)
model.to(device)
model.eval()

# Evaluate the model on the SQUAD dataset
f1s = []
exact_matches = []
with torch.no_grad():
    for batch in tqdm(dataloader):
        outputs = model(input_ids=batch["input_ids"].to(device),
                        labels=batch["labels"].to(device),
                        encoder_hidden_states=batch["encoder_hidden_states"].to(device))

        labels = batch["labels"]
        mask = labels == -100

        # since it's causal we predict the next token
        predicted_tokens = outputs.logits.argmax(dim=-1)[:, :-1]
        predicted_tokens[mask[:, 1:]] = tokenizer.pad_token_id
        predicted = tokenizer.batch_decode(predicted_tokens, skip_special_tokens=True)

        labels[mask] = tokenizer.pad_token_id
        ground_truth = tokenizer.batch_decode(labels, skip_special_tokens=True)
        print(f"Predicted: {predicted}")
        print(f"Ground truth: {ground_truth}")

        f1 = f1_score(predicted, ground_truth)
        exact_match = exact_match_score(predicted, ground_truth)

        f1s.extend(f1)
        exact_matches.extend(exact_match)


print(torch.tensor(f1s).mean())
print(torch.tensor(exact_matches).to(torch.float32).mean())