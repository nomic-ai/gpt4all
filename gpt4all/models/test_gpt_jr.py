import torch
from gpt4all.models import GPTJRForCausalLM, GPTJRConfig
from transformers import AutoTokenizer, AutoModel

# seed torch 

torch.manual_seed(0)

config = GPTJRConfig(encoder_dim=384, n_layer=4)
print("loaded config")

print("loading model")
model = GPTJRForCausalLM(config)
print("loaded model")


tokenizer =  AutoTokenizer.from_pretrained("EleutherAI/gpt-j-6b")
tokenizer.pad_token = tokenizer.eos_token

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
encoder_outputs = torch.stack([encodings, encodings]).squeeze().unsqueeze(0)

inputs = "What did the fox do?"

print("Encoded inputs")
tokenized_input = tokenizer([inputs], padding="max_length", truncation=True, return_tensors="pt")

print("Running model")
outputs = model(**tokenized_input, encoder_outputs=encoder_outputs)

print(outputs)
print(outputs[0].shape)

