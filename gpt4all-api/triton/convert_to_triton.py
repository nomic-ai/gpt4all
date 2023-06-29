import argparse
import os
from string import Template

import torch
from torch import nn
from transformers import AutoModelForCausalLM, AutoTokenizer

parser = argparse.ArgumentParser()

parser.add_argument(
    "--model", type=str, required=True, help="Path to HF checkpoint with the base model"
)

parser.add_argument(
    "--max-batch-size", type=int, default=64, help="Maximum batch size for inference"
)

parser.add_argument(
    "--revision",
    type=str,
    required=False,
    help="Optional branch/commit of the HF checkpoint",
)

parser.add_argument("--device", type=int, default=0)
args = parser.parse_args()

device = torch.device(args.device)


class ModelLogits(nn.Module):
    def __init__(self, model):
        super().__init__()
        self.model = model

    @torch.inference_mode()
    def forward(self, input_ids: torch.Tensor):
        return self.model(input_ids).logits


class InferModel(nn.Module):
    def __init__(self, traced_model, eos_token_id):
        super().__init__()
        self.traced_model = traced_model
        self.eos_token_id = eos_token_id

    def forward(
        self,
        input_ids: torch.Tensor,
        tensor_of_seq_len: torch.Tensor,
        temperature: torch.Tensor,
    ):
        # this has mostly been adapted from huggingface generate
        unfinished_sequences = torch.ones(input_ids.shape[0], dtype=torch.long, device=input_ids.device)
        eos_token_id_tensor = torch.tensor([self.eos_token_id]).to(input_ids.device)

        with torch.no_grad():
            for _ in range(tensor_of_seq_len.shape[1] - 1):
                logits = self.traced_model(input_ids).float()
                next_token_logits = logits[:, -1, :]
                next_token_logits = next_token_logits / temperature
                
                next_tokens = torch.multinomial(
                    torch.softmax(next_token_logits, dim=-1), input_ids.shape[0] 
                )

                next_tokens = next_tokens * unfinished_sequences + self.eos_token_id * (1 - unfinished_sequences)

                unfinished_sequences = unfinished_sequences.mul(
                    next_tokens.tile(eos_token_id_tensor.shape[0], 1).ne(eos_token_id_tensor.unsqueeze(1)).prod(dim=0)
                )

                # stop when each sentence is finished
                if unfinished_sequences.max() == 0:
                    return input_ids.int(), logits

                input_ids = torch.cat([input_ids, next_tokens], dim=-1)

                unfinished_sequences = unfinished_sequences.mul(
                    next_tokens.tile(eos_token_id_tensor.shape[0], 1).ne(eos_token_id_tensor.unsqueeze(1)).prod(dim=0)
                )

            # in TorchScript, the above logits var lifetime doesn't escape the loop's scope
            logits = self.traced_model(input_ids).float()
            next_token_logits = logits[:, -1, :]
            next_token_logits = next_token_logits / temperature

            next_tokens = torch.multinomial(
                    torch.softmax(next_token_logits, dim=-1), input_ids.shape[0] 
                )

            next_tokens = next_tokens * unfinished_sequences + self.eos_token_id * (1 - unfinished_sequences)

            input_ids = torch.cat([input_ids, next_tokens], dim=-1)

            return input_ids.int(), logits
            

print(f"Converting {args.model} to TorchScript...")
tokenizer = AutoTokenizer.from_pretrained(args.model, use_fast=False)
model = ModelLogits(AutoModelForCausalLM.from_pretrained(args.model,
                                                         trust_remote_code=True,
                                                         revision=args.revision,
                                                         torch_dtype=torch.float16,
                                                         use_cache=False))

model.eval()
model.requires_grad_(False)
model = model.to(device)


input = tokenizer("annotator model's hash is 0x", return_tensors="pt").to(device)
print(f"{model(input.input_ids)=}")

traced_script_module = torch.jit.trace(model, input.input_ids)
print("Tracing...")
print(f"{traced_script_module(input.input_ids)=}")

print("Scripting generation wrapper...")
# need to script this as we have data conditional flow
scripted_generator_model = torch.jit.script(InferModel(traced_script_module, tokenizer.eos_token_id))
print(scripted_generator_model.code)

print(f"{input.input_ids=}")
# x = input.input_ids, torch.empty(1, 5), torch.full([1, 1], 1.0).cuda(), torch.full([1, 1], len(tokenizer) // 2).cuda(), torch.full([1, 1], 0.9).cuda()
x = input.input_ids, torch.empty(1, 5),  torch.full([1, 1], 0.9).cuda()
print(x[0].shape)

print(f"{tokenizer.decode(scripted_generator_model(*x)[0][0])=}")

sanitized_name = args.model.replace("/", "--")
print("Model renamed to ", sanitized_name)

print("Saving TorchScript model...")

os.makedirs(f"model_store/{sanitized_name}/1", exist_ok=True)
scripted_generator_model.save(f"model_store/{sanitized_name}/1/traced-model.pt")

config_path = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), "triton_config.pbtxt"
)
with open(config_path) as f:
    template = Template(f.read())
config = template.substitute(
    {"model_name": sanitized_name, "max_batch_size": args.max_batch_size}
)
with open(f"model_store/{sanitized_name}/config.pbtxt", "w") as f:
    f.write(config)