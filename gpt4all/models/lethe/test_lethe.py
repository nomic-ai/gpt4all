import torch
from gpt4all.models import LetheForCausalLM, LetheConfig
from gpt4all.models.lethe.modeling_lethe import MemoryIndex
from transformers import AutoTokenizer, AutoModel

# seed torch 

torch.manual_seed(0)

config = LetheConfig(num_hidden_layers=12, 
                     hidden_size=1024, 
                     intermediate_size=4096, 
                     num_attention_heads=8,
                     cross_attn_layer=9,
                     nn_index_path="/home/paperspace/gpt4all/gpt4all/train",
                     num_neighbors_stored=32768,
                     num_neighbors_to_retrieve=2,
)
print("loaded config")

print("loading model")
dimension = config.max_position_embeddings * config.hidden_size
head_size = config.hidden_size // config.num_attention_heads
index = MemoryIndex(head_size,
                    64_000,
                    config.num_attention_heads
)
model = LetheForCausalLM(config, index)
print("loaded model")


tokenizer =  AutoTokenizer.from_pretrained("EleutherAI/pythia-1b")
tokenizer.pad_token = tokenizer.eos_token
tokenizer.model_max_length = 2048

question = "Where was George Washington born?"
answer  = "Virginia"

contexts = ["The Washington family was a wealthy Virginia planter family that had made its fortune through land speculation and the cultivation of tobacco.",
            "George Washington was born on February 22, 1732,[b] at Popes Creek in Westmoreland County, in the British colony of Virginia,[18] and was the first of six children of Augustine and Mary Ball Washington.",
            "His father was a justice of the peace and a prominent public figure who had four additional children from his first marriage to Jane Butler.[20] The family moved to Little Hunting Creek in 1735"]

contexts_encoded = tokenizer(contexts, padding="max_length", truncation=True, return_tensors="pt")
tokenized_input = tokenizer(question + "\n" + answer, return_tensors="pt", padding="max_length", truncation=True)

inputs = torch.concatenate([contexts_encoded["input_ids"], tokenized_input["input_ids"]], axis=0)
token_type_ids = torch.tensor([[0] * len(contexts_encoded["input_ids"]) + [1] * len(tokenized_input["input_ids"])]).squeeze()

question_len = len(tokenizer(question + "\n", return_tensors="pt")["input_ids"][0])

labels = inputs.clone()
labels[:-1] = -100
labels[-1, :question_len] = -100



print("Running model")
outputs = model(input_ids=inputs, token_type_ids=token_type_ids, labels=labels)

print(outputs)
print(outputs.logits.shape)

index.reset()
