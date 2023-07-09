"""
Python only API for running all GPT4All models.
"""
import os
import time
from contextlib import contextmanager
from pathlib import Path
from typing import Dict, Iterable, List, Union, Optional

import requests
from tqdm import tqdm

from . import pyllmodel

# TODO: move to config
DEFAULT_MODEL_DIRECTORY = os.path.join(str(Path.home()), ".cache", "gpt4all").replace("\\", "\\\\")

def embed(
    text: str
) -> list[float]:
    """
    Generate an embedding for all GPT4All.

    Args:
        text: The text document to generate an embedding for.

    Returns:
        An embedding of your document of text.
    """
    model = GPT4All(model_name='ggml-all-MiniLM-L6-v2-f16.bin')
    return model.model.generate_embedding(text)

class GPT4All:
    """
    Python class that handles instantiation, downloading, generation and chat with GPT4All models.
    """

    def __init__(
        self,
        model_name: str,
        model_path: Optional[str] = None,
        model_type: Optional[str] = None,
        allow_download: bool = True,
        n_threads: Optional[int] = None,
    ):
        """
        Constructor

        Args:
            model_name: Name of GPT4All or custom model. Including ".bin" file extension is optional but encouraged.
            model_path: Path to directory containing model file or, if file does not exist, where to download model.
                Default is None, in which case models will be stored in `~/.cache/gpt4all/`.
            model_type: Model architecture. This argument currently does not have any functionality and is just used as
                descriptive identifier for user. Default is None.
            allow_download: Allow API to download models from gpt4all.io. Default is True.
            n_threads: number of CPU threads used by GPT4All. Default is None, than the number of threads are determined automatically.
        """
        self.model_type = model_type
        self.model = pyllmodel.LLModel()
        # Retrieve model and download if allowed
        model_dest = self.retrieve_model(model_name, model_path=model_path, allow_download=allow_download)
        self.model.load_model(model_dest)
        # Set n_threads
        if n_threads is not None:
            self.model.set_thread_count(n_threads)

        self._is_chat_session_activated = False
        self.current_chat_session = []

    @staticmethod
    def list_models() -> Dict:
        """
        Fetch model list from https://gpt4all.io/models/models.json.

        Returns:
            Model list in JSON format.
        """
        return requests.get("https://gpt4all.io/models/models.json").json()

    @staticmethod
    def retrieve_model(
        model_name: str, model_path: Optional[str] = None, allow_download: bool = True, verbose: bool = True
    ) -> str:
        """
        Find model file, and if it doesn't exist, download the model.

        Args:
            model_name: Name of model.
            model_path: Path to find model. Default is None in which case path is set to
                ~/.cache/gpt4all/.
            allow_download: Allow API to download model from gpt4all.io. Default is True.
            verbose: If True (default), print debug messages.

        Returns:
            Model file destination.
        """

        model_filename = append_bin_suffix_if_missing(model_name)

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
            model_path = model_path.replace("\\", "\\\\")

        if not os.path.exists(model_path):
            raise ValueError(f"Invalid model directory: {model_path}")

        model_dest = os.path.join(model_path, model_filename).replace("\\", "\\\\")
        if os.path.exists(model_dest):
            if verbose:
                print("Found model file at ", model_dest)
            return model_dest

        # If model file does not exist, download
        elif allow_download:
            # Make sure valid model filename before attempting download
            available_models = GPT4All.list_models()

            selected_model = None
            for m in available_models:
                if model_filename == m['filename']:
                    selected_model = m
                    break

            if selected_model is None:
                raise ValueError(f"Model filename not in model list: {model_filename}")
            url = selected_model.pop('url', None)

            return GPT4All.download_model(model_filename, model_path, verbose=verbose, url=url)
        else:
            raise ValueError("Failed to retrieve model")

    @staticmethod
    def download_model(model_filename: str, model_path: str, verbose: bool = True, url: Optional[str] = None) -> str:
        """
        Download model from https://gpt4all.io.

        Args:
            model_filename: Filename of model (with .bin extension).
            model_path: Path to download model to.
            verbose: If True (default), print debug messages.
            url: the models remote url (e.g. may be hosted on HF)

        Returns:
            Model file destination.
        """

        def get_download_url(model_filename):
            if url:
                return url
            return f"https://gpt4all.io/models/{model_filename}"

        # Download model
        download_path = os.path.join(model_path, model_filename).replace("\\", "\\\\")
        download_url = get_download_url(model_filename)

        response = requests.get(download_url, stream=True)
        total_size_in_bytes = int(response.headers.get("content-length", 0))
        block_size = 2**20  # 1 MB

        with tqdm(total=total_size_in_bytes, unit="iB", unit_scale=True) as progress_bar:
            try:
                with open(download_path, "wb") as file:
                    for data in response.iter_content(block_size):
                        progress_bar.update(len(data))
                        file.write(data)
            except Exception:
                if os.path.exists(download_path):
                    if verbose:
                        print('Cleaning up the interrupted download...')
                    os.remove(download_path)
                raise

        # Validate download was successful
        if total_size_in_bytes != 0 and progress_bar.n != total_size_in_bytes:
            raise RuntimeError("An error occurred during download. Downloaded file may not work.")

        # Sleep for a little bit so OS can remove file lock
        time.sleep(2)

        if verbose:
            print("Model downloaded at: ", download_path)
        return download_path

    def generate(
        self,
        prompt: str,
        max_tokens: int = 200,
        temp: float = 0.7,
        top_k: int = 40,
        top_p: float = 0.1,
        repeat_penalty: float = 1.18,
        repeat_last_n: int = 64,
        n_batch: int = 8,
        n_predict: Optional[int] = None,
        streaming: bool = False,
    ) -> Union[str, Iterable]:
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

        Returns:
            Either the entire completion or a generator that yields the completion token by token.
        """
        generate_kwargs = dict(
            prompt=prompt,
            temp=temp,
            top_k=top_k,
            top_p=top_p,
            repeat_penalty=repeat_penalty,
            repeat_last_n=repeat_last_n,
            n_batch=n_batch,
            n_predict=n_predict if n_predict is not None else max_tokens,
        )

        if self._is_chat_session_activated:
            self.current_chat_session.append({"role": "user", "content": prompt})
            generate_kwargs['prompt'] = self._format_chat_prompt_template(messages=self.current_chat_session[-1:])
            generate_kwargs['reset_context'] = len(self.current_chat_session) == 1
        else:
            generate_kwargs['reset_context'] = True

        if streaming:
            return self.model.prompt_model_streaming(**generate_kwargs)

        output = self.model.prompt_model(**generate_kwargs)

        if self._is_chat_session_activated:
            self.current_chat_session.append({"role": "assistant", "content": output})

        return output

    @contextmanager
    def chat_session(self):
        '''
        Context manager to hold an inference optimized chat session with a GPT4All model.
        '''
        # Code to acquire resource, e.g.:
        self._is_chat_session_activated = True
        self.current_chat_session = []
        try:
            yield self
        finally:
            # Code to release resource, e.g.:
            self._is_chat_session_activated = False
            self.current_chat_session = []

    def _format_chat_prompt_template(
        self, messages: List[Dict], default_prompt_header=True, default_prompt_footer=True
    ) -> str:
        """
        Helper method for building a prompt using template from list of messages.

        Args:
            messages:  List of dictionaries. Each dictionary should have a "role" key
                with value of "system", "assistant", or "user" and a "content" key with a
                string value. Messages are organized such that "system" messages are at top of prompt,
                and "user" and "assistant" messages are displayed in order. Assistant messages get formatted as
                "Response: {content}".

        Returns:
            Formatted prompt.
        """
        full_prompt = ""

        for message in messages:
            if message["role"] == "user":
                user_message = "### Human: \n" + message["content"] + "\n### Assistant:\n"
                full_prompt += user_message
            if message["role"] == "assistant":
                assistant_message = message["content"] + '\n'
                full_prompt += assistant_message

        return full_prompt


def append_bin_suffix_if_missing(model_name):
    if not model_name.endswith(".bin"):
        model_name += ".bin"
    return model_name
