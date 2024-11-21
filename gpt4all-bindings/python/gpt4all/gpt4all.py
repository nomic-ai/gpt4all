"""
Python only API for running all GPT4All models.
"""
from __future__ import annotations

import hashlib
import json
import os
import platform
import re
import sys
import warnings
from contextlib import contextmanager
from datetime import datetime
from pathlib import Path
from types import TracebackType
from typing import TYPE_CHECKING, Any, Iterable, Iterator, Literal, NamedTuple, NoReturn, Protocol, TypedDict, overload

import jinja2
import requests
from jinja2.sandbox import ImmutableSandboxedEnvironment
from requests.exceptions import ChunkedEncodingError
from tqdm import tqdm
from urllib3.exceptions import IncompleteRead, ProtocolError

from ._pyllmodel import (CancellationError as CancellationError, EmbCancelCallbackType, EmbedResult as EmbedResult,
                         LLModel, ResponseCallbackType, _operator_call, empty_response_callback)

if TYPE_CHECKING:
    from typing_extensions import Self, TypeAlias

if sys.platform == "darwin":
    import fcntl

# TODO: move to config
DEFAULT_MODEL_DIRECTORY = Path.home() / ".cache" / "gpt4all"

ConfigType: TypeAlias = "dict[str, Any]"

# Environment setup adapted from HF transformers
@_operator_call
def _jinja_env() -> ImmutableSandboxedEnvironment:
    def raise_exception(message: str) -> NoReturn:
        raise jinja2.exceptions.TemplateError(message)

    def tojson(obj: Any, indent: int | None = None) -> str:
        return json.dumps(obj, ensure_ascii=False, indent=indent)

    def strftime_now(fmt: str) -> str:
        return datetime.now().strftime(fmt)

    env = ImmutableSandboxedEnvironment(trim_blocks=True, lstrip_blocks=True)
    env.filters["tojson"         ] = tojson
    env.globals["raise_exception"] = raise_exception
    env.globals["strftime_now"   ] = strftime_now
    return env


class MessageType(TypedDict):
    role: str
    content: str


class ChatSession(NamedTuple):
    template: jinja2.Template
    history: list[MessageType]


class Embed4All:
    """
    Python class that handles embeddings for GPT4All.
    """

    MIN_DIMENSIONALITY = 64

    def __init__(self, model_name: str | None = None, *, n_threads: int | None = None, device: str | None = None, **kwargs: Any):
        """
        Constructor

        Args:
            n_threads: number of CPU threads used by GPT4All. Default is None, then the number of threads are determined automatically.
            device: The processing unit on which the embedding model will run. See the `GPT4All` constructor for more info.
            kwargs: Remaining keyword arguments are passed to the `GPT4All` constructor.
        """
        if model_name is None:
            model_name = "all-MiniLM-L6-v2.gguf2.f16.gguf"
        self.gpt4all = GPT4All(model_name, n_threads=n_threads, device=device, **kwargs)

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self, typ: type[BaseException] | None, value: BaseException | None, tb: TracebackType | None,
    ) -> None:
        self.close()

    def close(self) -> None:
        """Delete the model instance and free associated system resources."""
        self.gpt4all.close()

    # return_dict=False
    @overload
    def embed(
        self, text: str, *, prefix: str | None = ..., dimensionality: int | None = ..., long_text_mode: str = ...,
        return_dict: Literal[False] = ..., atlas: bool = ..., cancel_cb: EmbCancelCallbackType | None = ...,
    ) -> list[float]: ...
    @overload
    def embed(
        self, text: list[str], *, prefix: str | None = ..., dimensionality: int | None = ..., long_text_mode: str = ...,
        return_dict: Literal[False] = ..., atlas: bool = ..., cancel_cb: EmbCancelCallbackType | None = ...,
    ) -> list[list[float]]: ...
    @overload
    def embed(
        self, text: str | list[str], *, prefix: str | None = ..., dimensionality: int | None = ...,
        long_text_mode: str = ..., return_dict: Literal[False] = ..., atlas: bool = ...,
        cancel_cb: EmbCancelCallbackType | None = ...,
    ) -> list[Any]: ...

    # return_dict=True
    @overload
    def embed(
        self, text: str, *, prefix: str | None = ..., dimensionality: int | None = ..., long_text_mode: str = ...,
        return_dict: Literal[True], atlas: bool = ..., cancel_cb: EmbCancelCallbackType | None = ...,
    ) -> EmbedResult[list[float]]: ...
    @overload
    def embed(
        self, text: list[str], *, prefix: str | None = ..., dimensionality: int | None = ..., long_text_mode: str = ...,
        return_dict: Literal[True], atlas: bool = ..., cancel_cb: EmbCancelCallbackType | None = ...,
    ) -> EmbedResult[list[list[float]]]: ...
    @overload
    def embed(
        self, text: str | list[str], *, prefix: str | None = ..., dimensionality: int | None = ...,
        long_text_mode: str = ..., return_dict: Literal[True], atlas: bool = ...,
        cancel_cb: EmbCancelCallbackType | None = ...,
    ) -> EmbedResult[list[Any]]: ...

    # return type unknown
    @overload
    def embed(
        self, text: str | list[str], *, prefix: str | None = ..., dimensionality: int | None = ...,
        long_text_mode: str = ..., return_dict: bool = ..., atlas: bool = ...,
        cancel_cb: EmbCancelCallbackType | None = ...,
    ) -> Any: ...

    def embed(
        self, text: str | list[str], *, prefix: str | None = None, dimensionality: int | None = None,
        long_text_mode: str = "mean", return_dict: bool = False, atlas: bool = False,
        cancel_cb: EmbCancelCallbackType | None = None,
    ) -> Any:
        """
        Generate one or more embeddings.

        Args:
            text: A text or list of texts to generate embeddings for.
            prefix: The model-specific prefix representing the embedding task, without the trailing colon. For Nomic
                Embed, this can be `search_query`, `search_document`, `classification`, or `clustering`. Defaults to
                `search_document` or equivalent if known; otherwise, you must explicitly pass a prefix or an empty
                string if none applies.
            dimensionality: The embedding dimension, for use with Matryoshka-capable models. Defaults to full-size.
            long_text_mode: How to handle texts longer than the model can accept. One of `mean` or `truncate`.
            return_dict: Return the result as a dict that includes the number of prompt tokens processed.
            atlas: Try to be fully compatible with the Atlas API. Currently, this means texts longer than 8192 tokens
                with long_text_mode="mean" will raise an error. Disabled by default.
            cancel_cb: Called with arguments (batch_sizes, backend_name). Return true to cancel embedding.

        Returns:
            With return_dict=False, an embedding or list of embeddings of your text(s).
            With return_dict=True, a dict with keys 'embeddings' and 'n_prompt_tokens'.

        Raises:
            CancellationError: If cancel_cb returned True and embedding was canceled.
        """
        if dimensionality is None:
            dimensionality = -1
        else:
            if dimensionality <= 0:
                raise ValueError(f"Dimensionality must be None or a positive integer, got {dimensionality}")
            if dimensionality < self.MIN_DIMENSIONALITY:
                warnings.warn(
                    f"Dimensionality {dimensionality} is less than the suggested minimum of {self.MIN_DIMENSIONALITY}."
                    " Performance may be degraded."
                )
        try:
            do_mean = {"mean": True, "truncate": False}[long_text_mode]
        except KeyError:
            raise ValueError(f"Long text mode must be one of 'mean' or 'truncate', got {long_text_mode!r}")
        result = self.gpt4all.model.generate_embeddings(text, prefix, dimensionality, do_mean, atlas, cancel_cb)
        return result if return_dict else result["embeddings"]


class GPT4All:
    """
    Python class that handles instantiation, downloading, generation and chat with GPT4All models.
    """

    def __init__(
        self,
        model_name: str,
        *,
        model_path: str | os.PathLike[str] | None = None,
        model_type: str | None = None,
        allow_download: bool = True,
        n_threads: int | None = None,
        device: str | None = None,
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
                - "gpu": Use Metal on ARM64 macOS, otherwise the same as "kompute".
                - "kompute": Use the best GPU provided by the Kompute backend.
                - "cuda": Use the best GPU provided by the CUDA backend.
                - "amd", "nvidia": Use the best GPU provided by the Kompute backend from this vendor.
                - A specific device name from the list returned by `GPT4All.list_gpus()`.
                Default is Metal on ARM64 macOS, "cpu" otherwise.

                Note: If a selected GPU device does not have sufficient RAM to accommodate the model, an error will be thrown, and the GPT4All instance will be rendered invalid. It's advised to ensure the device has enough memory before initiating the model.
            n_ctx: Maximum size of context window
            ngl: Number of GPU layers to use (Vulkan)
            verbose: If True, print debug messages.
        """

        self.model_type = model_type
        self._chat_session: ChatSession | None = None

        device_init = None
        if sys.platform == "darwin":
            if device is None:
                backend = "auto"  # "auto" is effectively "metal" due to currently non-functional fallback
            elif device == "cpu":
                backend = "cpu"
            else:
                if platform.machine() != "arm64" or device != "gpu":
                    raise ValueError(f"Unknown device for this platform: {device}")
                backend = "metal"
        else:
            backend = "kompute"
            if device is None or device == "cpu":
                pass  # use kompute with no device
            elif device in ("cuda", "kompute"):
                backend = device
                device_init = "gpu"
            elif device.startswith("cuda:"):
                backend = "cuda"
                device_init = _remove_prefix(device, "cuda:")
            else:
                device_init = _remove_prefix(device, "kompute:")

        # Retrieve model and download if allowed
        self.config: ConfigType = self.retrieve_model(model_name, model_path=model_path, allow_download=allow_download, verbose=verbose)
        self.model = LLModel(self.config["path"], n_ctx, ngl, backend)
        if device_init is not None:
            self.model.init_gpu(device_init)
        self.model.load_model()
        # Set n_threads
        if n_threads is not None:
            self.model.set_thread_count(n_threads)

    def __enter__(self) -> Self:
        return self

    def __exit__(
        self, typ: type[BaseException] | None, value: BaseException | None, tb: TracebackType | None,
    ) -> None:
        self.close()

    def close(self) -> None:
        """Delete the model instance and free associated system resources."""
        self.model.close()

    @property
    def backend(self) -> Literal["cpu", "kompute", "cuda", "metal"]:
        """The name of the llama.cpp backend currently in use. One of "cpu", "kompute", "cuda", or "metal"."""
        return self.model.backend

    @property
    def device(self) -> str | None:
        """The name of the GPU device currently in use, or None for backends other than Kompute or CUDA."""
        return self.model.device

    @property
    def current_chat_session(self) -> list[MessageType] | None:
        return None if self._chat_session is None else self._chat_session.history

    @current_chat_session.setter
    def current_chat_session(self, history: list[MessageType]) -> None:
        if self._chat_session is None:
            raise ValueError("current_chat_session may only be set when there is an active chat session")
        self._chat_session.history[:] = history

    @staticmethod
    def list_models() -> list[ConfigType]:
        """
        Fetch model list from https://gpt4all.io/models/models3.json.

        Returns:
            Model list in JSON format.
        """
        resp = requests.get("https://gpt4all.io/models/models3.json")
        if resp.status_code != 200:
            raise ValueError(f"Request failed: HTTP {resp.status_code} {resp.reason}")
        return resp.json()

    @classmethod
    def retrieve_model(
        cls,
        model_name: str,
        model_path: str | os.PathLike[str] | None = None,
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
        config: ConfigType = {}
        if allow_download:
            models = cls.list_models()
            if (model := next((m for m in models if m["filename"] == model_filename), None)) is not None:
                config.update(model)

        # Validate download directory
        if model_path is None:
            try:
                os.makedirs(DEFAULT_MODEL_DIRECTORY, exist_ok=True)
            except OSError as e:
                raise RuntimeError("Failed to create model download directory") from e
            model_path = DEFAULT_MODEL_DIRECTORY
        else:
            model_path = Path(model_path)

        if not model_path.exists():
            raise FileNotFoundError(f"Model directory does not exist: {model_path!r}")

        model_dest = model_path / model_filename
        if model_dest.exists():
            config["path"] = str(model_dest)
            if verbose:
                print(f"Found model file at {str(model_dest)!r}", file=sys.stderr)
        elif allow_download:
            # If model file does not exist, download
            filesize = config.get("filesize")
            config["path"] = str(cls.download_model(
                model_filename, model_path, verbose=verbose, url=config.get("url"),
                expected_size=None if filesize is None else int(filesize), expected_md5=config.get("md5sum"),
            ))
        else:
            raise FileNotFoundError(f"Model file does not exist: {model_dest!r}")

        return config

    @staticmethod
    def download_model(
        model_filename: str,
        model_path: str | os.PathLike[str],
        verbose: bool = True,
        url: str | None = None,
        expected_size: int | None = None,
        expected_md5: str | None = None,
    ) -> str | os.PathLike[str]:
        """
        Download model from gpt4all.io.

        Args:
            model_filename: Filename of model (with .gguf extension).
            model_path: Path to download model to.
            verbose: If True (default), print debug messages.
            url: the models remote url (e.g. may be hosted on HF)
            expected_size: The expected size of the download.
            expected_md5: The expected MD5 hash of the download.

        Returns:
            Model file destination.
        """

        # Download model
        if url is None:
            url = f"https://gpt4all.io/models/gguf/{model_filename}"

        def make_request(offset=None):
            headers = {}
            if offset:
                print(f"\nDownload interrupted, resuming from byte position {offset}", file=sys.stderr)
                headers["Range"] = f"bytes={offset}-"  # resume incomplete response
                headers["Accept-Encoding"] = "identity"  # Content-Encoding changes meaning of ranges
            response = requests.get(url, stream=True, headers=headers)
            if response.status_code not in (200, 206):
                raise ValueError(f"Request failed: HTTP {response.status_code} {response.reason}")
            if offset and (response.status_code != 206 or str(offset) not in response.headers.get("Content-Range", "")):
                raise ValueError("Connection was interrupted and server does not support range requests")
            if (enc := response.headers.get("Content-Encoding")) is not None:
                raise ValueError(f"Expected identity Content-Encoding, got {enc}")
            return response

        response = make_request()

        total_size_in_bytes = int(response.headers.get("content-length", 0))
        block_size = 2**20  # 1 MB

        partial_path = Path(model_path) / (model_filename + ".part")

        with open(partial_path, "w+b") as partf:
            try:
                with tqdm(desc="Downloading", total=total_size_in_bytes, unit="iB", unit_scale=True) as progress_bar:
                    while True:
                        last_progress = progress_bar.n
                        try:
                            for data in response.iter_content(block_size):
                                partf.write(data)
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
                                raise RuntimeError("Download not making progress, aborting.")
                            # server closed connection prematurely - retry
                            response = make_request(progress_bar.n)
                            continue
                        break

                # verify file integrity
                file_size = partf.tell()
                if expected_size is not None and file_size != expected_size:
                    raise ValueError(f"Expected file size of {expected_size} bytes, got {file_size}")
                if expected_md5 is not None:
                    partf.seek(0)
                    hsh = hashlib.md5()
                    with tqdm(desc="Verifying", total=file_size, unit="iB", unit_scale=True) as bar:
                        while chunk := partf.read(block_size):
                            hsh.update(chunk)
                            bar.update(len(chunk))
                    if hsh.hexdigest() != expected_md5.lower():
                        raise ValueError(f"Expected MD5 hash of {expected_md5!r}, got {hsh.hexdigest()!r}")
            except:
                if verbose:
                    print("Cleaning up the interrupted download...", file=sys.stderr)
                try:
                    os.remove(partial_path)
                except OSError:
                    pass
                raise

            # flush buffers and sync the inode
            partf.flush()
            _fsync(partf)

        # move to final destination
        download_path = Path(model_path) / model_filename
        try:
            os.rename(partial_path, download_path)
        except FileExistsError:
            try:
                os.remove(partial_path)
            except OSError:
                pass
            raise

        if verbose:
            print(f"Model downloaded to {str(download_path)!r}", file=sys.stderr)
        return download_path

    @overload
    def generate(
        self, prompt: str, *, max_tokens: int = ..., temp: float = ..., top_k: int = ..., top_p: float = ...,
        min_p: float = ..., repeat_penalty: float = ..., repeat_last_n: int = ..., n_batch: int = ...,
        n_predict: int | None = ..., streaming: Literal[False] = ..., callback: ResponseCallbackType = ...,
    ) -> str: ...
    @overload
    def generate(
        self, prompt: str, *, max_tokens: int = ..., temp: float = ..., top_k: int = ..., top_p: float = ...,
        min_p: float = ..., repeat_penalty: float = ..., repeat_last_n: int = ..., n_batch: int = ...,
        n_predict: int | None = ..., streaming: Literal[True], callback: ResponseCallbackType = ...,
    ) -> Iterable[str]: ...
    @overload
    def generate(
        self, prompt: str, *, max_tokens: int = ..., temp: float = ..., top_k: int = ..., top_p: float = ...,
        min_p: float = ..., repeat_penalty: float = ..., repeat_last_n: int = ..., n_batch: int = ...,
        n_predict: int | None = ..., streaming: bool, callback: ResponseCallbackType = ...,
    ) -> Any: ...

    def generate(
        self,
        prompt         : str,
        *,
        max_tokens     : int                  = 200,
        temp           : float                = 0.7,
        top_k          : int                  = 40,
        top_p          : float                = 0.4,
        min_p          : float                = 0.0,
        repeat_penalty : float                = 1.18,
        repeat_last_n  : int                  = 64,
        n_batch        : int                  = 8,
        n_predict      : int | None           = None,
        streaming      : bool                 = False,
        callback       : ResponseCallbackType = empty_response_callback,
    ) -> Any:
        """
        Generate outputs from any GPT4All model.

        Args:
            prompt: The prompt for the model to complete.
            max_tokens: The maximum number of tokens to generate.
            temp: The model temperature. Larger values increase creativity but decrease factuality.
            top_k: Randomly sample from the top_k most likely tokens at each generation step. Set this to 1 for greedy decoding.
            top_p: Randomly sample at each generation step from the top most likely tokens whose probabilities add up to top_p.
            min_p: Randomly sample at each generation step from the top most likely tokens whose probabilities are at least min_p.
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
        generate_kwargs: dict[str, Any] = dict(
            temp           = temp,
            top_k          = top_k,
            top_p          = top_p,
            min_p          = min_p,
            repeat_penalty = repeat_penalty,
            repeat_last_n  = repeat_last_n,
            n_batch        = n_batch,
            n_predict      = n_predict if n_predict is not None else max_tokens,
        )

        # Prepare the callback, process the model response
        full_response = ""

        def _callback_wrapper(token_id: int, response: str) -> bool:
            nonlocal full_response
            full_response += response
            return callback(token_id, response)

        last_msg_rendered = prompt
        if self._chat_session is not None:
            session = self._chat_session
            def render(messages: list[MessageType]) -> str:
                return session.template.render(
                    messages=messages,
                    add_generation_prompt=True,
                    **self.model.special_tokens_map,
                )
            session.history.append(MessageType(role="user", content=prompt))
            prompt = render(session.history)
            if len(session.history) > 1:
                last_msg_rendered = render(session.history[-1:])

        # Check request length
        last_msg_len = self.model.count_prompt_tokens(last_msg_rendered)
        if last_msg_len > (limit := self.model.n_ctx - 4):
            raise ValueError(f"Your message was too long and could not be processed ({last_msg_len} > {limit}).")

        # Send the request to the model
        if streaming:
            def stream() -> Iterator[str]:
                yield from self.model.prompt_model_streaming(prompt, _callback_wrapper, **generate_kwargs)
                if self._chat_session is not None:
                    self._chat_session.history.append(MessageType(role="assistant", content=full_response))
            return stream()

        self.model.prompt_model(prompt, _callback_wrapper, **generate_kwargs)
        if self._chat_session is not None:
            self._chat_session.history.append(MessageType(role="assistant", content=full_response))
        return full_response

    @contextmanager
    def chat_session(
        self,
        system_message: str | Literal[False] | None = None,
        chat_template: str | None = None,
    ):
        """
        Context manager to hold an inference optimized chat session with a GPT4All model.

        Args:
            system_message: An initial instruction for the model, None to use the model default, or False to disable. Defaults to None.
            chat_template: Jinja template for the conversation, or None to use the model default. Defaults to None.
        """

        if system_message is None:
            system_message = self.config.get("systemMessage", False)

        if chat_template is None:
            if "name" not in self.config:
                raise ValueError("For sideloaded models or with allow_download=False, you must specify a chat template.")
            if "chatTemplate" not in self.config:
                raise NotImplementedError("This model appears to have a built-in chat template, but loading it is not "
                                          "currently implemented. Please pass a template to chat_session() directly.")
            if (tmpl := self.config["chatTemplate"]) is None:
                raise ValueError(f"The model {self.config['name']!r} does not support chat.")
            chat_template = tmpl

        history = []
        if system_message is not False:
            history.append(MessageType(role="system", content=system_message))
        self._chat_session = ChatSession(
            template=_jinja_env.from_string(chat_template),
            history=history,
        )
        try:
            yield self
        finally:
            self._chat_session = None

    @staticmethod
    def list_gpus() -> list[str]:
        """
        List the names of the available GPU devices.

        Returns:
            A list of strings representing the names of the available GPU devices.
        """
        return LLModel.list_gpus()


def append_extension_if_missing(model_name):
    if not model_name.endswith((".bin", ".gguf")):
        model_name += ".gguf"
    return model_name


class _HasFileno(Protocol):
    def fileno(self) -> int: ...


def _fsync(fd: int | _HasFileno) -> None:
    if sys.platform == "darwin":
        # Apple's fsync does not flush the drive write cache
        try:
            fcntl.fcntl(fd, fcntl.F_FULLFSYNC)
        except OSError:
            pass  # fall back to fsync
        else:
            return
    os.fsync(fd)


def _remove_prefix(s: str, prefix: str) -> str:
    return s[len(prefix):] if s.startswith(prefix) else s
