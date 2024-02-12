"""
Python only API for running all GPT4All models.
"""
from __future__ import annotations

import os
import sys
import time
from contextlib import contextmanager
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Union

import requests
from requests.exceptions import ChunkedEncodingError
from tqdm import tqdm
from urllib3.exceptions import IncompleteRead, ProtocolError

from . import _pyllmodel

# TODO: move to config
DEFAULT_MODEL_DIRECTORY = os.path.join(str(Path.home()), ".cache", "gpt4all").replace("\\", "\\\\")

DEFAULT_MODEL_CONFIG = {
    "systemPrompt": "",
    "promptTemplate": "### Human: \n{0}\n### Assistant:\n",
}

ConfigType = Dict[str, str]
MessageType = Dict[str, str]


class Embed4All:
    """
    Python class that handles embeddings for GPT4All.
    """

    def __init__(self, model_name: Optional[str] = None, n_threads: Optional[int] = None, **kwargs):
        """
        Constructor

        Args:
            n_threads: number of CPU threads used by GPT4All. Default is None, then the number of threads are determined automatically.
        """
        self.gpt4all = GPT4All(model_name or 'all-MiniLM-L6-v2-f16.gguf', n_threads=n_threads, **kwargs)

    def embed(self, text: str) -> List[float]:
        """
        Generate an embedding.

        Args:
            text: The text document to generate an embedding for.

        Returns:
            An embedding of your document of text.
        """
        return self.gpt4all.model.generate_embedding(text)


class GPT4All:
    """
    Python class that handles instantiation, downloading, generation and chat with GPT4All models.
    """

    def __init__(
        self,
        model_name: str,
        model_path: Optional[Union[str, os.PathLike[str]]] = None,
        model_type: Optional[str] = None,
        allow_download: bool = True,
        n_threads: Optional[int] = None,
        device: Optional[str] = "cpu",
        n_ctx: int = 2048,
        ngl: int = 100,
        verbose: bool = False,
    ):
        """
        Constructor

        Args:
            model_name: Name of GPT4All or custom model. Including ".gguf" file extension is optional but encouraged.
            model_path: Path to directory containing model file or, if file does not exist, where to download model.
                Default is None, in which case models will be stored in `~/.cache/gpt4all/`.
            model_type: Model architecture. This argument currently does not have any functionality and is just used as
                descriptive identifier for user. Default is None.
            allow_download: Allow API to download models from gpt4all.io. Default is True.
            n_threads: number of CPU threads used by GPT4All. Default is None, then the number of threads are determined automatically.
            device: The processing unit on which the GPT4All model will run. It can be set to:
                - "cpu": Model will run on the central processing unit.
                - "gpu": Model will run on the best available graphics processing unit, irrespective of its vendor.
                - "amd", "nvidia", "intel": Model will run on the best available GPU from the specified vendor.
                Alternatively, a specific GPU name can also be provided, and the model will run on the GPU that matches the name if it's available.
                Default is "cpu".

                Note: If a selected GPU device does not have sufficient RAM to accommodate the model, an error will be thrown, and the GPT4All instance will be rendered invalid. It's advised to ensure the device has enough memory before initiating the model.
            n_ctx: Maximum size of context window
            ngl: Number of GPU layers to use (Vulkan)
            verbose: If True, print debug messages.
        """
        self.model_type = model_type
        # Retrieve model and download if allowed
        self.config: ConfigType = self.retrieve_model(model_name, model_path=model_path, allow_download=allow_download, verbose=verbose)
        self.model = _pyllmodel.LLModel(self.config["path"], n_ctx, ngl)
        if device is not None and device != "cpu":
            self.model.init_gpu(device)
        self.model.load_model()
        # Set n_threads
        if n_threads is not None:
            self.model.set_thread_count(n_threads)

        self._is_chat_session_activated: bool = False
        self.current_chat_session: List[MessageType] = empty_chat_session()
        self._current_prompt_template: str = "{0}"

    @staticmethod
    def list_models() -> List[ConfigType]:
        """
        Fetch model list from https://gpt4all.io/models/models2.json.

        Returns:
            Model list in JSON format.
        """
        resp = requests.get("https://gpt4all.io/models/models2.json")
        if resp.status_code != 200:
            raise ValueError(f'Request failed: HTTP {resp.status_code} {resp.reason}')
        return resp.json()

    @staticmethod
    def retrieve_model(
        model_name: str,
        model_path: Optional[Union[str, os.PathLike[str]]] = None,
        allow_download: bool = True,
        verbose: bool = False,
    ) -> ConfigType:
        """
        Find model file, and if it doesn't exist, download the model.

        Args:
            model_name: Name of model.
            model_path: Path to find model. Default is None in which case path is set to
                ~/.cache/gpt4all/.
            allow_download: Allow API to download model from gpt4all.io. Default is True.
            verbose: If True (default), print debug messages.

        Returns:
            Model config.
        """

        model_filename = append_extension_if_missing(model_name)

        # get the config for the model
        config: ConfigType = DEFAULT_MODEL_CONFIG
        if allow_download:
            available_models = GPT4All.list_models()

            for m in available_models:
                if model_filename == m["filename"]:
                    config.update(m)
                    config["systemPrompt"] = config["systemPrompt"].strip()
                    config["promptTemplate"] = config["promptTemplate"].replace(
                        "%1", "{0}", 1
                    )  # change to Python-style formatting
                    break

        # Validate download directory
        if model_path is None:
            try:
                os.makedirs(DEFAULT_MODEL_DIRECTORY, exist_ok=True)
            except OSError as exc:
                raise ValueError(
                    f"Failed to create model download directory at {DEFAULT_MODEL_DIRECTORY}: {exc}. "
                    "Please specify model_path."
                )
            model_path = DEFAULT_MODEL_DIRECTORY
        else:
            model_path = str(model_path).replace("\\", "\\\\")

        if not os.path.exists(model_path):
            raise ValueError(f"Invalid model directory: {model_path}")

        model_dest = os.path.join(model_path, model_filename).replace("\\", "\\\\")
        if os.path.exists(model_dest):
            config.pop("url", None)
            config["path"] = model_dest
            if verbose:
                print("Found model file at", model_dest, file=sys.stderr)

        # If model file does not exist, download
        elif allow_download:
            url = config.pop("url", None)

            config["path"] = GPT4All.download_model(model_filename, model_path, verbose=verbose, url=url)
        else:
            raise ValueError("Failed to retrieve model")

        return config

    @staticmethod
    def download_model(
        model_filename: str,
        model_path: Union[str, os.PathLike[str]],
        verbose: bool = True,
        url: Optional[str] = None,
    ) -> str:
        """
        Download model from https://gpt4all.io.

        Args:
            model_filename: Filename of model (with .gguf extension).
            model_path: Path to download model to.
            verbose: If True (default), print debug messages.
            url: the models remote url (e.g. may be hosted on HF)

        Returns:
            Model file destination.
        """

        def get_download_url(model_filename):
            if url:
                return url
            return f"https://gpt4all.io/models/gguf/{model_filename}"

        # Download model
        download_path = os.path.join(model_path, model_filename).replace("\\", "\\\\")
        download_url = get_download_url(model_filename)

        def make_request(offset=None):
            headers = {}
            if offset:
                print(f"\nDownload interrupted, resuming from byte position {offset}", file=sys.stderr)
                headers['Range'] = f'bytes={offset}-'  # resume incomplete response
            response = requests.get(download_url, stream=True, headers=headers)
            if response.status_code not in (200, 206):
                raise ValueError(f'Request failed: HTTP {response.status_code} {response.reason}')
            if offset and (response.status_code != 206 or str(offset) not in response.headers.get('Content-Range', '')):
                raise ValueError('Connection was interrupted and server does not support range requests')
            return response

        response = make_request()

        total_size_in_bytes = int(response.headers.get("content-length", 0))
        block_size = 2**20  # 1 MB

        with open(download_path, "wb") as file, \
                tqdm(total=total_size_in_bytes, unit="iB", unit_scale=True) as progress_bar:
            try:
                while True:
                    last_progress = progress_bar.n
                    try:
                        for data in response.iter_content(block_size):
                            file.write(data)
                            progress_bar.update(len(data))
                    except ChunkedEncodingError as cee:
                        if cee.args and isinstance(pe := cee.args[0], ProtocolError):
                            if len(pe.args) >= 2 and isinstance(ir := pe.args[1], IncompleteRead):
                                assert progress_bar.n <= ir.partial  # urllib3 may be ahead of us but never behind
                                # the socket was closed during a read - retry
                                response = make_request(progress_bar.n)
                                continue
                        raise
                    if total_size_in_bytes != 0 and progress_bar.n < total_size_in_bytes:
                        if progress_bar.n == last_progress:
                            raise RuntimeError('Download not making progress, aborting.')
                        # server closed connection prematurely - retry
                        response = make_request(progress_bar.n)
                        continue
                    break
            except Exception:
                if verbose:
                    print("Cleaning up the interrupted download...", file=sys.stderr)
                try:
                    os.remove(download_path)
                except OSError:
                    pass
                raise

        if os.name == 'nt':
            time.sleep(2)  # Sleep for a little bit so Windows can remove file lock

        if verbose:
            print("Model downloaded at:", download_path, file=sys.stderr)
        return download_path

    def generate(
        self,
        prompt: str,
        max_tokens: int = 200,
        temp: float = 0.7,
        top_k: int = 40,
        top_p: float = 0.4,
        repeat_penalty: float = 1.18,
        repeat_last_n: int = 64,
        n_batch: int = 8,
        n_predict: Optional[int] = None,
        streaming: bool = False,
        callback: _pyllmodel.ResponseCallbackType = _pyllmodel.empty_response_callback,
    ) -> Union[str, Iterable[str]]:
        """
        Generate outputs from any GPT4All model.

        Args:
            prompt: The prompt for the model the complete.
            max_tokens: The maximum number of tokens to generate.
            temp: The model temperature. Larger values increase creativity but decrease factuality.
            top_k: Randomly sample from the top_k most likely tokens at each generation step. Set this to 1 for greedy decoding.
            top_p: Randomly sample at each generation step from the top most likely tokens whose probabilities add up to top_p.
            repeat_penalty: Penalize the model for repetition. Higher values result in less repetition.
            repeat_last_n: How far in the models generation history to apply the repeat penalty.
            n_batch: Number of prompt tokens processed in parallel. Larger values decrease latency but increase resource requirements.
            n_predict: Equivalent to max_tokens, exists for backwards compatibility.
            streaming: If True, this method will instead return a generator that yields tokens as the model generates them.
            callback: A function with arguments token_id:int and response:str, which receives the tokens from the model as they are generated and stops the generation by returning False.

        Returns:
            Either the entire completion or a generator that yields the completion token by token.
        """

        # Preparing the model request
        generate_kwargs: Dict[str, Any] = dict(
            temp=temp,
            top_k=top_k,
            top_p=top_p,
            repeat_penalty=repeat_penalty,
            repeat_last_n=repeat_last_n,
            n_batch=n_batch,
            n_predict=n_predict if n_predict is not None else max_tokens,
        )

        if self._is_chat_session_activated:
            # check if there is only one message, i.e. system prompt:
            generate_kwargs["reset_context"] = len(self.current_chat_session) == 1
            self.current_chat_session.append({"role": "user", "content": prompt})

            prompt = self._format_chat_prompt_template(
                messages=self.current_chat_session[-1:],
                default_prompt_header=self.current_chat_session[0]["content"]
                if generate_kwargs["reset_context"]
                else "",
            )
        else:
            generate_kwargs["reset_context"] = True

        # Prepare the callback, process the model response
        output_collector: List[MessageType]
        output_collector = [
            {"content": ""}
        ]  # placeholder for the self.current_chat_session if chat session is not activated

        if self._is_chat_session_activated:
            self.current_chat_session.append({"role": "assistant", "content": ""})
            output_collector = self.current_chat_session

        def _callback_wrapper(
            callback: _pyllmodel.ResponseCallbackType,
            output_collector: List[MessageType],
        ) -> _pyllmodel.ResponseCallbackType:
            def _callback(token_id: int, response: str) -> bool:
                nonlocal callback, output_collector

                output_collector[-1]["content"] += response

                return callback(token_id, response)

            return _callback

        # Send the request to the model
        if streaming:
            return self.model.prompt_model_streaming(
                prompt=prompt,
                callback=_callback_wrapper(callback, output_collector),
                **generate_kwargs,
            )

        self.model.prompt_model(
            prompt=prompt,
            callback=_callback_wrapper(callback, output_collector),
            **generate_kwargs,
        )

        return output_collector[-1]["content"]

    @contextmanager
    def chat_session(
        self,
        system_prompt: str = "",
        prompt_template: str = "",
    ):
        """
        Context manager to hold an inference optimized chat session with a GPT4All model.

        Args:
            system_prompt: An initial instruction for the model.
            prompt_template: Template for the prompts with {0} being replaced by the user message.
        """
        # Code to acquire resource, e.g.:
        self._is_chat_session_activated = True
        self.current_chat_session = empty_chat_session(system_prompt or self.config["systemPrompt"])
        self._current_prompt_template = prompt_template or self.config["promptTemplate"]
        try:
            yield self
        finally:
            # Code to release resource, e.g.:
            self._is_chat_session_activated = False
            self.current_chat_session = empty_chat_session()
            self._current_prompt_template = "{0}"

    def _format_chat_prompt_template(
        self,
        messages: List[MessageType],
        default_prompt_header: str = "",
        default_prompt_footer: str = "",
    ) -> str:
        """
        Helper method for building a prompt from list of messages using the self._current_prompt_template as a template for each message.

        Args:
            messages:  List of dictionaries. Each dictionary should have a "role" key
                with value of "system", "assistant", or "user" and a "content" key with a
                string value. Messages are organized such that "system" messages are at top of prompt,
                and "user" and "assistant" messages are displayed in order. Assistant messages get formatted as
                "Response: {content}".

        Returns:
            Formatted prompt.
        """

        if isinstance(default_prompt_header, bool):
            import warnings

            warnings.warn(
                "Using True/False for the 'default_prompt_header' is deprecated. Use a string instead.",
                DeprecationWarning,
            )
            default_prompt_header = ""

        if isinstance(default_prompt_footer, bool):
            import warnings

            warnings.warn(
                "Using True/False for the 'default_prompt_footer' is deprecated. Use a string instead.",
                DeprecationWarning,
            )
            default_prompt_footer = ""

        full_prompt = default_prompt_header + "\n\n" if default_prompt_header != "" else ""

        for message in messages:
            if message["role"] == "user":
                user_message = self._current_prompt_template.format(message["content"])
                full_prompt += user_message
            if message["role"] == "assistant":
                assistant_message = message["content"] + "\n"
                full_prompt += assistant_message

        full_prompt += "\n\n" + default_prompt_footer if default_prompt_footer != "" else ""

        return full_prompt


def empty_chat_session(system_prompt: str = "") -> List[MessageType]:
    return [{"role": "system", "content": system_prompt}]


def append_extension_if_missing(model_name):
    if not model_name.endswith((".bin", ".gguf")):
        model_name += ".gguf"
    return model_name
