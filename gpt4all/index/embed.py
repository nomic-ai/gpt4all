import torch
from transformers import AutoTokenizer, AutoModel
import torch.nn.functional as F


class Embedder:
    def __init__(self, model_name="sentence-transformers/all-MiniLM-L6-v2"):
        self.tokenizer = AutoTokenizer.from_pretrained(model_name)
        self.embedder = AutoModel.from_pretrained(model_name)
        # hack
        self.offset = self.tokenizer.model_max_length // 2

    def _mean_pool(self, model_output, attention_mask):
        token_embeddings = model_output[
            0
        ]  # First element of model_output contains all token embeddings
        input_mask_expanded = (
            attention_mask.unsqueeze(-1).expand(token_embeddings.size()).float()
        )
        sentence_embeddings = torch.sum(
            token_embeddings * input_mask_expanded, 1
        ) / torch.clamp(input_mask_expanded.sum(1), min=1e-9)
        return F.normalize(sentence_embeddings, p=2, dim=1)

    def chunk_text(self, text):
        tokenized_text = {"input_ids": [], "attention_mask": []}
        tokenized = self.tokenizer(text)
        tokenized_len = len(tokenized["input_ids"])
        max_len = self.tokenizer.model_max_length
        if tokenized_len > max_len:
            start = 0
            while start < tokenized_len:
                tokenized_text["input_ids"].append(
                    tokenized["input_ids"][start : start + max_len]
                )
                tokenized_text["attention_mask"].append(
                    tokenized["attention_mask"][start : start + max_len]
                )
                # this could probably be done better
                start += self.offset

        else:
            tokenized_text["input_ids"].append(tokenized["input_ids"])
            tokenized_text["attention_mask"].append(tokenized["attention_mask"])

        return tokenized_text

    def __call__(self, batch):
        if isinstance(batch, dict):
            outputs = self.embedder(
                input_ids=batch["input_ids"], attention_mask=batch["attention_mask"]
            )
            embedding = self._mean_pool(outputs, batch["attention_mask"])

            return {"id": batch["id"], "embedding": embedding}

        elif isinstance(batch, str):
            tokenized = self.tokenizer(batch, return_tensors="pt", truncation=True)
            return self._mean_pool(
                self.embedder(
                    input_ids=tokenized["input_ids"],
                    attention_mask=tokenized["attention_mask"],
                ),
                tokenized["attention_mask"],
            )

    def to(self, device):
        self.embedder.to(device)
