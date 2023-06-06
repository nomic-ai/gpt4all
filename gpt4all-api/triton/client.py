import torch
import tritonclient.grpc.aio as grpcclient


def prepare_inference_inputs(
    inputs_ids: torch.IntTensor, new_tokens: int = 1, temperature: float = 1.0, top_k: int = 0, top_p: float = 1.0
):
    batch_size = inputs_ids.shape[0]

    input_ids_input = grpcclient.InferInput("input_ids", inputs_ids.shape, "INT32")
    input_ids_input.set_data_from_numpy(inputs_ids.int().cpu().numpy())

    new_tokens_input = grpcclient.InferInput(
        "tensor_of_seq_len", [batch_size, new_tokens], "INT32"
    )
    new_tokens_input.set_data_from_numpy(
        torch.zeros(batch_size, new_tokens, dtype=torch.int32).cpu().numpy()
    )

    temperature_input = grpcclient.InferInput("temperature", [batch_size, 1], "FP32")
    temperature_input.set_data_from_numpy(
        torch.full([batch_size, 1], temperature, dtype=torch.float32).cpu().numpy()
    )

    top_k_input = grpcclient.InferInput("top_k", [batch_size, 1], "INT32")
    top_k_input.set_data_from_numpy(
        torch.full([batch_size, 1], top_k, dtype=torch.int32).cpu().numpy()
    )

    top_p_input = grpcclient.InferInput("top_p", [batch_size, 1], "FP32")
    top_p_input.set_data_from_numpy(
        torch.full([batch_size, 1], top_p, dtype=torch.float32).cpu().numpy()
    )

    inputs = [input_ids_input, new_tokens_input, temperature_input, top_k_input, top_p_input]
    outputs = [
        grpcclient.InferRequestedOutput("logits"),
        grpcclient.InferRequestedOutput("output_ids"),
    ]
    return inputs, outputs


async def infer(
    triton_client, model_name, input_ids, new_tokens: int = 1, temperature: float = 1.0, top_k: int = 0, top_p: float = 1.0
):
    inputs, outputs = prepare_inference_inputs(input_ids, new_tokens, temperature, top_k, top_p)

    triton_model_name = model_name.replace("/", "--")

    result = await triton_client.infer(
        model_name=triton_model_name, inputs=inputs, outputs=outputs
    )

    logits = torch.tensor(result.as_numpy("logits").copy(), requires_grad=False)
    output_ids = torch.tensor(result.as_numpy("output_ids").copy(), requires_grad=False)

    return logits, output_ids

def Client(url: str):
  return grpcclient.InferenceServerClient(url=url)

if __name__ == "__main__":
    import argparse
    from transformers import AutoTokenizer

    parser = argparse.ArgumentParser()
    parser.add_argument("--url", type=str, default="localhost:8001")
    parser.add_argument("--model", type=str, default="gpt2")

    args = parser.parse_args()

    tokenizer = AutoTokenizer.from_pretrained(args.model)

    async def main():
        async with Client(args.url) as triton_client:
            while True:
                prompt = input("Prompt: ")
                input_ids = tokenizer.encode(prompt, return_tensors="pt")
                last_logits, output_ids = await infer(
                    triton_client, args.model, input_ids, new_tokens=128, temperature=1.0, top_k=0, top_p=0.9,
                )
                print(tokenizer.decode(output_ids[0]))

    import asyncio
    asyncio.run(main())