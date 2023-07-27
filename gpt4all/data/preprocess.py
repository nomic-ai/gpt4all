import torch

def tokenize_inputs(config, tokenizer, examples, input_col, target_col):
    max_length = config["max_length"]

    # hacky backward compatible
    different_eos = tokenizer.eos_token != "</s>"
    out = {"labels": [], "input_ids": [], "attention_mask": []}
    for prompt, response in zip(examples[input_col], examples[target_col]):
        if different_eos:
            if response.count("</s> \n") > 0:
                response = response.replace("</s> \n", f"{tokenizer.eos_token} \n") 

        prompt_len = len(tokenizer(prompt + "\n", return_tensors="pt")["input_ids"][0])

        # hack if our prompt is super long
        # we need to include some labels so we arbitrarily trunacate at max_length // 2
        # if the length is too long
        if prompt_len >= max_length // 2:
            # if prompt is too long, truncate
            # but make sure to truncate to at max 1024 tokens
            new_len = min(max_length // 2, len(prompt) // 2)
            prompt = prompt[:new_len]
            # get new prompt length
            prompt_len = tokenizer(prompt + "\n", return_tensors="pt", max_length=max_length // 2, truncation=True).input_ids.ne(tokenizer.pad_token_id).sum().item()

        assert prompt_len <= max_length // 2, f"prompt length {prompt_len} exceeds max length {max_length}"

        input_tokens = tokenizer(prompt + "\n" + response + tokenizer.eos_token,
                                 truncation=True, max_length=max_length, return_tensors="pt")["input_ids"].squeeze()

        labels = input_tokens.clone()
        labels[:prompt_len] = -100
        if len(labels) < max_length:
            # pad to max_length with -100
            labels = torch.cat([labels, torch.full((max_length - len(labels),), -100)])

        assert (labels == -100).sum() < len(labels), f"Labels are all -100, something wrong. prompt length {prompt_len} exceeds max length {max_length}" 
        
        if (labels == -100).sum() == len(labels) - 1:
            print(prompt)
            print(response)
            raise

        tokenized = tokenizer.pad({"input_ids": input_tokens}, padding="max_length", max_length=max_length)
        out["labels"].append(labels)
        out["input_ids"].append(tokenized["input_ids"])
        out["attention_mask"].append(tokenized["attention_mask"])

    out = {k: torch.stack(v) if isinstance(v, list) else v for k, v in out.items()}

    return out