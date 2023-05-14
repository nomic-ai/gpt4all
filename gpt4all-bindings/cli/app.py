import sys
import typer

from typing_extensions import Annotated
from gpt4all import GPT4All

MESSAGES = [
    {"role": "system", "content": "You are a helpful assistant."},
    {"role": "user", "content": "Hello there."},
    {"role": "assistant", "content": "Hi, how can I help you?"},
]

SPECIAL_COMMANDS = {
    "/reset": lambda messages: messages.clear(),
    "/exit": lambda _: sys.exit(),
    "/clear": lambda _: print("\n" * 100),
    "/help": lambda _: print("Special commands: /reset, /exit, /help and /clear"),
}

VERSION = "0.1.0"

CLI_START_MESSAGE = f"""
    
 ██████  ██████  ████████ ██   ██  █████  ██      ██      
██       ██   ██    ██    ██   ██ ██   ██ ██      ██      
██   ███ ██████     ██    ███████ ███████ ██      ██      
██    ██ ██         ██         ██ ██   ██ ██      ██      
 ██████  ██         ██         ██ ██   ██ ███████ ███████ 
                                                          

Welcome to the GPT4All CLI! Version {VERSION}
Type /help for special commands.
                                                    
"""

def _cli_override_response_callback(token_id, response):
    resp = response.decode("utf-8")
    print(resp, end="", flush=True)
    return True


# create typer app
app = typer.Typer()

@app.command()
def repl(
    model: Annotated[
        str,
        typer.Option("--model", "-m", help="Model to use for chatbot"),
    ] = "ggml-gpt4all-j-v1.3-groovy",
    n_threads: Annotated[
        int,
        typer.Option("--n-threads", "-t", help="Number of threads to use for chatbot"),
    ] = 4,
):
    gpt4all_instance = GPT4All(model)

    # if threads are passed, set them
    if n_threads != 4:
        num_threads = gpt4all_instance.model.thread_count()
        print(f"\nAdjusted: {num_threads} →", end="")

        # set number of threads
        gpt4all_instance.model.set_thread_count(n_threads)

        num_threads = gpt4all_instance.model.thread_count()
        print(f" {num_threads} threads", end="", flush=True)


    # overwrite _response_callback on model
    gpt4all_instance.model._response_callback = _cli_override_response_callback

    print(CLI_START_MESSAGE)

    while True:
        message = input(" ⇢  ")

        # Check if special command and take action
        if message in SPECIAL_COMMANDS:
            SPECIAL_COMMANDS[message](MESSAGES)
            continue

        # if regular message, append to messages
        MESSAGES.append({"role": "user", "content": message})

        # execute chat completion and ignore the full response since 
        # we are outputting it incrementally
        full_response = gpt4all_instance.chat_completion(
            MESSAGES,
            # preferential kwargs for chat ux
            logits_size=0,
            tokens_size=0,
            n_past=0,
            n_ctx=0,
            n_predict=200,
            top_k=40,
            top_p=0.9,
            temp=0.9,
            n_batch=9,
            repeat_penalty=1.1,
            repeat_last_n=64,
            context_erase=0.0,
            # required kwargs for cli ux (incremental response)
            verbose=False,
            std_passthrough=True,
        )
        # record assistant's response to messages
        MESSAGES.append(full_response.get("choices")[0].get("message"))
        print() # newline before next prompt


@app.command()
def version():
    print("gpt4all-cli v0.1.0")


if __name__ == "__main__":
    app()
