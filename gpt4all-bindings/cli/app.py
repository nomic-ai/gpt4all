"""GPT4All CLI

The GPT4All CLI is a self-contained script based on the `gpt4all` and `typer` packages. It offers a
REPL to communicate with a language model similar to the chat GUI application, but more basic.
"""

import io
import pkg_resources  # should be present as a dependency of gpt4all
import logging
import signal
import sys
import typer

from collections import namedtuple
from typing_extensions import Annotated
from gpt4all import GPT4All


logging.basicConfig()

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
Type /help for special commands.
                                                    
"""

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
    ] = None,
):
    """The CLI read-eval-print loop."""
    gpt4all_instance = GPT4All(model)

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
    #_setup_signal_handler(gpt4all_instance)
    manage_sigint = ResponseSigintManager(gpt4all_instance)

    print(CLI_START_MESSAGE)

    use_new_loop = False
    try:
        version = pkg_resources.Environment()['gpt4all'][0].version
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

        # handle SIGINT gracefully during chat_completion
        #_activate_response_sigint_handler()
        with manage_sigint:

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
                streaming=True,
            )

        # revert to default SIGINT
        #_deactivate_response_sigint_handler()

        # record assistant's response to messages
        MESSAGES.append(full_response.get("choices")[0].get("message"))
        print() # newline before next prompt


def _new_loop(gpt4all_instance):
    with gpt4all_instance.chat_session():
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
            response_generator = gpt4all_instance.generate(
                message,
                # preferential kwargs for chat ux
                max_tokens=200,
                temp=0.9,
                top_k=40,
                top_p=0.9,
                repeat_penalty=1.1,
                repeat_last_n=64,
                n_batch=9,
                # required kwargs for cli ux (incremental response)
                streaming=True,
            )
            response = io.StringIO()
            for token in response_generator:
                print(token, end='', flush=True)
                response.write(token)

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



###################
# Signal Handling #
###################

# proof of concept
# TODO:
# - turn it into a context manager
# - may want to refactor the CLI itself instead of messing around with globals
# - may want to find a better way to change the '_response_callback'

_keep_generating = False
_old_sigint_handler = None

def _response_callback(token_id, response):
    sys.stdout.write(response.decode('utf-8', 'replace'))
    global _keep_generating
    return _keep_generating

def _setup_signal_handler(gpt4all_instance):
    # overriding the callback in an ugly hack because the API is not very flexible:
    gpt4all_instance.model._response_callback = _response_callback
    global _old_sigint_handler
    _old_sigint_handler = signal.getsignal(signal.SIGINT)

def _halt_response_sigint_handler(signal, frame):
    global _keep_generating
    _keep_generating = False

def _activate_response_sigint_handler():
    signal.signal(signal.SIGINT, _halt_response_sigint_handler)
    global _keep_generating
    _keep_generating = True

def _deactivate_response_sigint_handler():
    global _old_sigint_handler
    signal.signal(signal.SIGINT, _old_sigint_handler)

class ResponseSigintManager:
    # TODO: docstrings; should care be taken for it to be reentrant?
    # TODO: might also have to make sure that the terminal prompt is reset properly
    # note: the default behaviour if something goes wrong with patching/activating is to let the
    #       response keep generating

    def __init__(self, gpt4all: GPT4All):
        if not isinstance(gpt4all, GPT4All):
            raise TypeError(f"'gpt4all' must be of type 'gpt4all.GPT4All', but is '{type(gpt4all)}'.")
        self._gpt4all = gpt4all
        self._is_response_callback_patched = False
        self._old_response_callback = None
        self._old_sigint_handler = None
        self._keep_generating_response = True

    @property
    def gpt4all(self):
        return self._gpt4all

    @property
    def is_managing_sigint(self):
        # invariant: old handler is stored <-> own handler is active
        return self._old_sigint_handler is not None

    @property
    def keep_generating_response(self):
        return self._keep_generating

    def __enter__(self):
        if not self.gpt4all:
            raise RuntimeError()
        if not self._is_response_callback_patched:
            self._patch_response_callback()
        self._activate_response_sigint_handler()
    
    def __exit__(self, exc_type, exc_value, traceback):
        self._deactivate_response_sigint_handler()
        self._revert_response_callback()

    def _patch_response_callback(self):
        try:
            self.gpt4all.model._response_callback = self._response_callback
            self._is_response_callback_patched = True
            return True
        except Exception as exc:
            logging.warn("Unable to patch '_response_callback'. SIGINT will not be handled. Cause: {exc}")
            return False
    
    def _revert_response_callback(self):
        if self._is_response_callback_patched:
            try:
                self.gpt4all.model._response_callback = self._old_response_callback
            except Exception as exc:
                logging.warn("Unable to revert '_response_callback'. The GPT4All API"
                             " might remain in an inconsistent state. Cause: {exc}")
            finally:
                # revert state on a best-effort basis; reversal shouldn't fail under normal circumstances:
                self._old_response_callback = None
                self._is_response_callback_patched = False

    def _response_callback(self, token_id, response):
        sys.stdout.write(response.decode('utf-8', 'replace'))
        return self.keep_generating

    def _activate_response_sigint_handler(self):
        if self._is_response_callback_patched and self._old_sigint_handler is None:
            try:
                self._old_sigint_handler = signal.signal(signal.SIGINT, self._halt_response_sigint_handler)  # TODO
                return True
            except Exception as exc:
                logging.warn("Unable to activate the response SIGINT handler. Cause: {exc}")
                return False
    
    def _deactivate_response_sigint_handler(self):
        self._keep_generating_response = True
        if self.is_managing_sigint:
            try:
                signal.signal(signal.SIGINT, self._old_sigint_handler)
                self._old_sigint_handler = None
            except Exception as exc:
                logging.warn("Unable to deactivate the response SIGINT handler. The handling"
                             " of SIGINT might remain in an inconsistent state. Cause: {exc}")
    
    def _halt_response_sigint_handler(signal_, frame):
        assert signal_ == signal.SIGINT, f"expected signal.SIGINT ({signal.SIGINT}) but got {signal_.name} ({signal_})"
        self._keep_generating_response = False
    
    def __del__(self):
        if self.is_managing_sigint:
            self._deactivate_response_sigint_handler()
        if self.gpt4all:
            self._revert_response_callback()
        self._gpt4all = None

    def __repr__(self):
        return f'<{self.__class__.__name__}(gpt4all={self.gpt4all})>'
    
    def __str__(self):
        return ("{class_name} for {gpt4all}; '_response_handler()' patched: {is_patched};"
                " currently managing SIGINT: {is_managing_sigint}").format(
                    class_name=self.__class__.__name__,
                    gpt4all=self.gpt4all,
                    is_patched=('yes' if self._is_response_callback_patched else 'no'),
                    is_managing_sigint=('yes' if self.is_managing_sigint else 'no'))



if __name__ == "__main__":
    try:
        app()
    except KeyboardInterrupt:
        print("\n\nKeyboard interrupt received, exiting.")
    except Exception as exc:
        sys.exit(exc)
