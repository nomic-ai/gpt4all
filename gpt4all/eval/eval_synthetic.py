import torch
from gpt4all.data.instruction_tuning_dataloader import load_data
from gpt4all.utils.read import read_config
from transformers import AutoTokenizer, AutoModelForCausalLM
from argparse import ArgumentParser
from tqdm import tqdm

parser = ArgumentParser()
parser.add_argument("--config", type=str, default="config.yaml")

args = parser.parse_args()

config = read_config(args.config)

tokenizer = AutoTokenizer.from_pretrained(config["tokenizer_name"])
if tokenizer.pad_token is None:
    tokenizer.pad_token = tokenizer.eos_token

train_dataloader, val_dataloader = load_data(config, tokenizer) 


model = AutoModelForCausalLM.from_pretrained(config["model_name"], 
                                                trust_remote_code=True) 

device = torch.device("cuda" if torch.cuda.is_available() else "cpu")

model = model.half().to(device)
model.eval()

# Evaluate the model on the SQUAD dataset
f1s = []
exact_matches = []
with torch.no_grad():
    for batch in tqdm(val_dataloader):
        inputs = batch["input_ids"].to(device)
        labels = batch["labels"].to(device)
        cutoff = torch.argmax((labels != -100).type(torch.float32))
        outputs = model.generate(inputs[:, :cutoff], max_new_tokens=100)
        print(f"Predicted: {tokenizer.batch_decode(outputs, skip_special_tokens=True)}")
        print(f"Ground truth: {tokenizer.batch_decode(inputs[:, cutoff:], skip_special_tokens=True)}")
        print(tokenizer.batch_decode(inputs, skip_special_tokens=True))

