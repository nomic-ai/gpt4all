#!/usr/bin/env python3
"""GPT4All CLI

The GPT4All CLI is a self-contained script based on the `gpt4all` and `typer` packages. It offers a
REPL to communicate with a language model similar to the chat GUI application, but more basic.
"""

import time
import threading
import importlib.metadata
import io
import sys
from collections import namedtuple
from typing_extensions import Annotated

import typer
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

VersionInfo = namedtuple('VersionInfo', ['major', 'minor', 'micro'])
VERSION_INFO = VersionInfo(1, 0, 2)
VERSION = '.'.join(map(str, VERSION_INFO))  # convert to string form, like: '1.2.3'

CLI_START_MESSAGE = f"""
    
 ██████  ██████  ████████ ██   ██  █████  ██      ██      
██       ██   ██    ██    ██   ██ ██   ██ ██      ██      
██   ███ ██████     ██    ███████ ███████ ██      ██      
██    ██ ██         ██         ██ ██   ██ ██      ██      
 ██████  ██         ██         ██ ██   ██ ███████ ███████ 
                                                          

Welcome to the GPT4All CLI! Version {VERSION}
Type '.<enter>' on its own line to submit a prompt or command
Type /help for special commands.
                                                    
"""

# create typer app
app = typer.Typer()

@app.command()
def repl(
    model: Annotated[
        str,
        typer.Option("--model", "-m", help="Model to use for chatbot"),
    ] = "mistral-7b-instruct-v0.1.Q4_0.gguf",
    n_threads: Annotated[
        int,
        typer.Option("--n-threads", "-t", help="Number of threads to use for chatbot"),
    ] = None,
    device: Annotated[
        str,
        typer.Option("--device", "-d", help="Device to use for chatbot, e.g. gpu, amd, nvidia, intel. Defaults to CPU."),
    ] = None,
):
    """The CLI read-eval-print loop."""
    gpt4all_instance = GPT4All(model, device=device)

    # if threads are passed, set them
    if n_threads is not None:
        num_threads = gpt4all_instance.model.thread_count()
        print(f"\nAdjusted: {num_threads} →", end="")

        # set number of threads
        gpt4all_instance.model.set_thread_count(n_threads)

        num_threads = gpt4all_instance.model.thread_count()
        print(f" {num_threads} threads", end="", flush=True)
    else:
        print(f"\nUsing {gpt4all_instance.model.thread_count()} threads", end="")

    print(CLI_START_MESSAGE)

    use_new_loop = False
    try:
        version = importlib.metadata.version('gpt4all')
        version_major = int(version.split('.')[0])
        if version_major >= 1:
            use_new_loop = True
    except:
        pass  # fall back to old loop
    if use_new_loop:
        _new_loop(gpt4all_instance)
    else:
        _old_loop(gpt4all_instance)


def _old_loop(gpt4all_instance):
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
            min_p=0.0,
            temp=0.9,
            n_batch=9,
            repeat_penalty=1.1,
            repeat_last_n=64,
            context_erase=0.0,
            # required kwargs for cli ux (incremental response)
            verbose=False,
            streaming=True,
        )
        # record assistant's response to messages
        MESSAGES.append(full_response.get("choices")[0].get("message"))
        print()  # newline before next prompt


def _new_loop(gpt4all_instance):
    processing_done = threading.Event()

    with gpt4all_instance.chat_session():
        while True:
            loading_thread = threading.Thread(
                target=display_processing_animation, args=(processing_done,)
            )

            message = get_prompt()

            # Check if special command and take action
            if message in SPECIAL_COMMANDS:
                SPECIAL_COMMANDS[message](MESSAGES)
                processing_done.set()
                continue
            else:
                loading_thread.start()

            # if regular message, append to messages
            MESSAGES.append({"role": "user", "content": message})

            # execute chat completion and ignore the full response since 
            # we are outputting it incrementally
            response_generator = gpt4all_instance.generate(
                message,
                # preferential kwargs for chat ux
                max_tokens=200,
                temp=0.9,
                top_k=40,
                top_p=0.9,
                min_p=0.0,
                repeat_penalty=1.1,
                repeat_last_n=64,
                n_batch=9,
                # required kwargs for cli ux (incremental response)
                streaming=True,
            )
            response = io.StringIO()

            # Consider processing complete when first token is received
            first = True
            for token in response_generator:
                if first:
                    processing_done.set()
                    loading_thread.join()
                    first = False

                print(token, end="", flush=True)
                response.write(token)

            # reset the 'processing' flag ready for next prompt
            processing_done.clear

            # record assistant's response to messages
            response_message = {'role': 'assistant', 'content': response.getvalue()}
            response.close()
            gpt4all_instance.current_chat_session.append(response_message)
            MESSAGES.append(response_message)
            print() # newline before next prompt


@app.command()
def version():
    """The CLI version command."""
    print(f"gpt4all-cli v{VERSION}")


def get_prompt():
    message = ""
    print(" ⇢  ")
    while True:
        line = input()
        # Special commands on any line override the entire prompt
        if line in SPECIAL_COMMANDS:
            message = line
            break
        # /. is a special command that ends and submits the prompt
        if line.strip() == "/.":
            break
        else:
            message += line

    return message


def display_processing_animation(processing_done):
    processing_animation = ["|", "/", "-", "\\"]
    display_processing_feedback = True
    processing_done.clear()
    a = 0
    while display_processing_feedback:
        time.sleep(0.2)
        a = a + 1
        print(
            "\r " + processing_animation[a % len(processing_animation)] + "\r",
            end="",
        )

        if processing_done.is_set():
            display_processing_feedback = False
            print(" 🤖" + " " * 100, end="\n")


if __name__ == "__main__":
    app()
