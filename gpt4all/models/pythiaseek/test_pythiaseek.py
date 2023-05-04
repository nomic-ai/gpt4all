import torch
from gpt4all.models import PythiaSeekConfig, PythiaSeekForCausalLM
from transformers import AutoTokenizer, AutoModel

# seed torch 

torch.manual_seed(0)

config = PythiaSeekConfig(encoder_dim=384, 
                          hidden_size=1024, 
                          intermediate_size=4096,
                          num_hidden_layers=4,
                          num_attention_heads=4)
print("loaded config")

print("loading model")
model = PythiaSeekForCausalLM(config)
print("loaded model")


tokenizer =  AutoTokenizer.from_pretrained("EleutherAI/pythia-1b")
tokenizer.pad_token = tokenizer.eos_token
tokenizer.model_max_length = 1024

encoder_tokenizer = AutoTokenizer.from_pretrained('sentence-transformers/all-MiniLM-L6-v2')
encoder = AutoModel.from_pretrained('sentence-transformers/all-MiniLM-L6-v2')


def mean_pooling(model_output, attention_mask):
    token_embeddings = model_output[0] #First element of model_output contains all token embeddings
    input_mask_expanded = attention_mask.unsqueeze(-1).expand(token_embeddings.size()).float()
    return torch.sum(token_embeddings * input_mask_expanded, 1) / torch.clamp(input_mask_expanded.sum(1), min=1e-9)

text = "The quick brown fox jumps over the lazy dog."
print("Encoded knn")
tokenized = encoder_tokenizer(text, return_tensors="pt")

# bs, seq_len, dim
encodings = mean_pooling(encoder(**tokenized), tokenized["attention_mask"])

# make 2 neighbors
# (bs, knn, encoding_dim)
encoder_hidden_states = torch.stack([encodings, encodings]).squeeze().unsqueeze(0)

inputs = "What did the fox do?"

print("Encoded inputs")
tokenized_input = tokenizer([inputs], padding="max_length", truncation=True, return_tensors="pt")

print("Running model")
outputs = model(**tokenized_input, encoder_hidden_states=encoder_hidden_states)

print(outputs)
print(outputs[0].shape)

