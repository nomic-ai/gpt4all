from gpt4all import GPT4All
import sys

def do_prompt(model:GPT4All, prompt:str):
    print(f">>> prompt: `{prompt}`")
    for chunk in model.generate(prompt, temp=0.0, streaming=True):
        print(chunk, end='')
        sys.stdout.flush()
    print()

def main():
    model = GPT4All('ggml-mpt-7b-chat.bin', model_type='mpt')
    # outside of a session the kvcache is reset after every prompt
    with model.chat_session():
        print(">>> capture kv cache")
        do_prompt(model, "Who was the 13th president of the USA?")
        kvcopy = model.model.copy_kv_cache()
        do_prompt(model, "Who was the 15th president of the USA?")
        do_prompt(model, "What years was he in office?")
        print(">>> restore kv cache")
        model.model.set_kv_cache(kvcopy)
        do_prompt(model, "What years was he in office?")


if __name__ == "__main__":
    main()
