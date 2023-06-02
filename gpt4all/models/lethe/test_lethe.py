import torch
from gpt4all.models import LetheForCausalLM, LetheConfig
from gpt4all.models.lethe.modeling_lethe import MemoryIndex
from transformers import AutoTokenizer, AutoModel
from datasets import load_from_disk
from tqdm import tqdm

# seed torch 

torch.manual_seed(0)

config = LetheConfig(num_hidden_layers=15, 
                     hidden_size=2048, 
                     intermediate_size=8192, 
                     num_attention_heads=8,
                     memory_attn_layer=12,
                     num_neighbors_stored=6_000_000,
                     num_neighbors_to_retrieve=32,
)
print("loaded config")

print("loading model")
dimension = config.max_position_embeddings * config.hidden_size
head_size = config.hidden_size // config.num_attention_heads
index = MemoryIndex(head_size,
                    5_000_000,
                    # 2 since multi-query attention and storing one each for key and value
                    config.num_attention_heads,
)
model = LetheForCausalLM(config, index)
model.to("cuda:0")
print("loaded model")


tokenizer =  AutoTokenizer.from_pretrained("EleutherAI/pythia-1b")
tokenizer.pad_token = tokenizer.eos_token
tokenizer.model_max_length = 2048

dataset = load_from_disk("/home/paperspace/gpt4all/gpt4all/index/squad_supplemented_train")

item = dataset[0]

question = item["question"]
answer = item["answers"]["text"][0]
print(f"question: {question}")
print(f"answer: {answer}")

contexts = item["neighbor_text"]

contexts_encoded = tokenizer(contexts, padding="max_length", truncation=True, return_tensors="pt")
tokenized_input = tokenizer(question + "\n" + answer, return_tensors="pt", padding="max_length", truncation=True)

inputs = torch.concatenate([contexts_encoded["input_ids"], tokenized_input["input_ids"]], axis=0)
token_type_ids = torch.tensor([[0] * len(contexts_encoded["input_ids"]) + [1] * len(tokenized_input["input_ids"])]).squeeze()

question_len = len(tokenizer(question + "\n", return_tensors="pt")["input_ids"][0])

labels = inputs.clone()
labels[:-1] = -100
labels[-1, :question_len] = -100


memory_mask = token_type_ids == 0
# should be shape (num_memories, sequence_length)
memories = inputs[memory_mask]

print("Running model")
model.eval()
with torch.no_grad():
    for chunk_start in tqdm(range(0, memories.shape[0], 32)):
        chunk_end = min(memories.shape[0], chunk_start + 32)
        mem_chunk = memories[chunk_start:chunk_end].to(model.device)
        model(input_ids=mem_chunk, labels=None,)

model.train()

qa_inputs = inputs[~memory_mask]
qa_labels = labels[~memory_mask]
outputs = model(input_ids=qa_inputs.to(model.device), labels=qa_labels.to(model.device))

print(outputs)
print(outputs.logits.shape)

index.reset()
