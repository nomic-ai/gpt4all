import torch
import torch.nn.functional as F
from gpt4all.models import LetheForCausalLM
from gpt4all.models.lethe.modeling_lethe import MemoryIndex
from gpt4all.data.retrieval_dataloader import load_memory_augmented_data
from gpt4all.train.metrics import f1_score, exact_match_score
from gpt4all.utils.read import read_config
from transformers import AutoTokenizer, AutoConfig
from argparse import ArgumentParser
from tqdm import tqdm
from nomic import atlas
from datasets import load_from_disk


def calc_loss_per_item(logits, labels):
    lm_logits = logits[:, :-1, :].contiguous()
    lm_labels = labels[:, 1:].contiguous()
    loss = F.cross_entropy(lm_logits.view(-1, lm_logits.size(-1)), lm_labels.view(-1), reduction="none")
    loss = loss.reshape(labels.shape[0], -1).mean(dim=-1)

    # return tensor of shape (B,) where B is the batch size
    return loss.cpu().tolist()

    
def greedy_search(input_ids, model, tokenizer, max_new_tokens=100):
    num_new_tokens = 0
    with torch.no_grad():
        while True:
            if num_new_tokens >= max_new_tokens:
                break
            outputs = model(input_ids, save_kv=False)

            new_tokens = torch.argmax(outputs.logits[:, -1, :], dim=-1)

            input_ids = torch.cat([input_ids, new_tokens.unsqueeze(1)], dim=-1)
            num_new_tokens += 1

            if torch.equal(input_ids[0, -1].cpu(), torch.tensor(tokenizer.eos_token_id)):
                break
        
    print(tokenizer.batch_decode(input_ids, skip_special_tokens=True))

    return input_ids
    

parser = ArgumentParser()
parser.add_argument("--config", type=str, default="config.yaml")

args = parser.parse_args()

config = read_config(args.config)

tokenizer = AutoTokenizer.from_pretrained(config["tokenizer_name"], model_max_length=config["max_length"])
if tokenizer.pad_token is None:
    tokenizer.pad_token = tokenizer.eos_token

dataloader = load_memory_augmented_data(config, tokenizer, split_dataset=False)

dataset = load_from_disk(config["dataset_path"])


device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model_config = AutoConfig.from_pretrained(config["model_name"])

head_size = model_config.hidden_size // model_config.num_attention_heads
index = MemoryIndex(head_size,
            config["num_memories_per_index"],
            model_config.num_attention_heads
)
model = LetheForCausalLM.from_pretrained(config["model_name"], 
                                    revision=config['version'] if 'version' in config else None,
                                    memory_attn_layer=config["memory_attn_layer"],
                                    num_neighbors_to_retrieve=config["num_neighbors_to_retrieve"],
                                    index=index,
                                ).to(device)
model.eval()

# Evaluate the model on the SQUAD dataset
losses = []
with torch.no_grad():
    for i, batch in enumerate(tqdm(dataloader)):
        memories = batch["retrieved_context"]
        memories = memories[:, :config["num_neighbors_to_store"], :]
        memories = memories.reshape(-1, memories.shape[-1])

        # need to set to eval so we don't do mem attn as it's slow
        with torch.no_grad():
            for chunk_start in range(0, memories.shape[0], config["mem_chunk_size"]):
                chunk_end = min(memories.shape[0], chunk_start + config["mem_chunk_size"])
                mem_chunk = memories[chunk_start:chunk_end]
                model(input_ids=mem_chunk.to(device))

        del memories
        torch.cuda.empty_cache()
        qa_inputs = batch["input_ids"]
        qa_labels = batch["labels"]
        for i in range(qa_inputs.shape[0]):
            inputs = qa_inputs[i].to(device)
            labels = qa_labels[i].to(device)
            cutoff = torch.argmax((labels != -100).type(torch.float32))
            greedy_search(inputs[:cutoff.item()].unsqueeze(0).to(device), model, tokenizer)
            print(tokenizer.decode(inputs, skip_special_tokens=True))


        # batch_loss = calc_loss_per_item(outputs.logits, qa_labels.to(device))
        # losses.extend(batch_loss)
        index.reset()

        
        
dataset = dataset.add_column("loss", losses)

dataset.save_to_disk("eval_squad_atlas_map")


        
