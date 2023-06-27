"""
Python only API for running all GPT4All models.
"""
import openai

import os
from pathlib import Path
import time
from typing import Dict, List

import requests
from tqdm import tqdm

from . import pyllmodel

# TODO: move to config
DEFAULT_MODEL_DIRECTORY = os.path.join(str(Path.home()), ".cache", "gpt4all").replace("\\", "\\\\")


class GPT4All():
    """Python API for retrieving and interacting with GPT4All models.
    
    Attributes:
        model: Pointer to underlying C model.
    """
    def __init__(self, model_name: str, api_key: str = None, model_path: str = None, model_type: str = None, allow_download = True):
        """
        Constructor

        Args:
            model_name: Name of GPT4All or custom model. Including ".bin" file extension is optional but encouraged.
            model_path: Path to directory containing model file or, if file does not exist, where to download model.
                Default is None, in which case models will be stored in `~/.cache/gpt4all/`.
            model_type: Model architecture. This argument currently does not have any functionality and is just used as
                descriptive identifier for user. Default is None.
            allow_download: Allow API to download models from gpt4all.io. Default is True.
        """
        self.api_key = api_key
        self.openai = openai.api_key = api_key if api_key else None

        self.model_type = model_type
        self.model = None

        if self.api_key:
            self.model_name = model_name
        else:
            self.model = pyllmodel.LLModel()
            model_dest = self.retrieve_model(model_name, model_path=model_path, allow_download=allow_download)
            self.model.load_model(model_dest)
    @staticmethod
    def list_models():
        """
        Fetch model list from https://gpt4all.io/models/models.json.

        Returns:
            Model list in JSON format.
        """
        return requests.get("https://gpt4all.io/models/models.json").json()


    @staticmethod
    def retrieve_model(model_name: str, model_path: str = None, allow_download: bool = True, verbose: bool = True) -> str:
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

        if model_path is None:
            try:
                os.makedirs(DEFAULT_MODEL_DIRECTORY, exist_ok=True)
            except OSError as exc:
                raise ValueError(f"Failed to create model download directory at {DEFAULT_MODEL_DIRECTORY}: {exc}. "
                                 "Please specify model_path.")
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
        elif allow_download:
            available_models = GPT4All.list_models()
            if model_filename not in (m["filename"] for m in available_models):
                raise ValueError(f"Model filename not in model list: {model_filename}")
            return GPT4All.download_model(model_filename, model_path, verbose=verbose)
        else:
            raise ValueError("Failed to retrieve model")

    @staticmethod
    def download_model(model_filename: str, model_path: str, verbose: bool = True) -> str:
        """
        Download a model file.

        Args:
            model_filename: Name of model file.
            model_path: Path to store downloaded model file.
            verbose: If True (default), print debug messages.

        Returns:
            Destination of the downloaded model file.
        """
        def get_download_url(model_filename):
            return f"https://gpt4all.io/models/{model_filename}"

        download_path = os.path.join(model_path, model_filename).replace("\\", "\\\\")
        download_url = get_download_url(model_filename)

        response = requests.get(download_url, stream=True)
        total_size_in_bytes = int(response.headers.get("content-length", 0))
        block_size = 2 ** 20  # 1 MB

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

        if total_size_in_bytes != 0 and progress_bar.n != total_size_in_bytes:
            raise RuntimeError("An error occurred during download. Downloaded file may not work.")

        time.sleep(2)

        if verbose:
            print("Model downloaded at: ", download_path)
        return download_path

    def generate(self, messages: List[Dict], full_prompt: str = None, streaming: bool = True, **generate_kwargs) -> str:
        """
        Generate a text response using a list of messages or a full prompt.

        Args:
            messages: List of messages to generate response from.
            full_prompt: Full string prompt to generate response from. Default is None.
            streaming: If True (default), use streaming for generation.
            **generate_kwargs: Additional keyword arguments for generate function.

        Returns:
            Generated text response.
        """
        if self.api_key:
            response = openai.ChatCompletion.create(
                model=self.model_name,
                messages=messages,
                **generate_kwargs
            )
            return response
        else:
            return self.model.generate(full_prompt, streaming=streaming, **generate_kwargs)

    def chat_completion(self,
                        messages: List[Dict],
                        default_prompt_header: bool = True,
                        default_prompt_footer: bool = True,
                        verbose: bool = True,
                        streaming: bool = True,
                        **generate_kwargs) -> dict:
        """
        Complete a chat using a list of messages.

        Args:
            messages: List of messages to complete the chat from.
            default_prompt_header: If True (default), add a default prompt header.
            default_prompt_footer: If True, add a default prompt footer. Default is False.
            verbose: If True (default), print debug messages.
            streaming: If True (default), use streaming for completion.
            **generate_kwargs: Additional keyword arguments for generate function.

        Returns:
            Response dictionary containing model details and generated response.
        """
        full_prompt = self._build_prompt(messages,
                                         default_prompt_header=default_prompt_header,
                                         default_prompt_footer=default_prompt_footer)
        if verbose:
            print(full_prompt)

        response = self.generate(messages, full_prompt=full_prompt, streaming=streaming, **generate_kwargs)

        if verbose and not streaming:
            print(response)

        if self.api_key:
            return response

        response_dict = {
            "model": self.model_name if self.api_key else self.model.model_name,
            "usage": {"prompt_tokens": len(full_prompt),
                      "completion_tokens": len(response),
                      "total_tokens": len(full_prompt) + len(response)},
            "choices": [
                {
                    "message": {
                        "role": "assistant",
                        "content": response
                    }
                }
            ]
        }

        return response_dict

    @staticmethod
    def _build_prompt(messages: List[Dict],
                      default_prompt_header=True,
                      default_prompt_footer=False) -> str:
        """
        Build a full prompt from a list of messages.

        Args:
            messages: List of messages to build the prompt from.
            default_prompt_header: If True (default), add a default prompt header.
            default_prompt_footer: If True, add a default prompt footer. Default is False.

        Returns:
            Full prompt string.
        """
        full_prompt = ""

        for message in messages:
            if message["role"] == "system":
                system_message = message["content"] + "\n"
                full_prompt += system_message

        if default_prompt_header:
            full_prompt += """### Instruction: 
            The prompt below is a question to answer, a task to complete, or a conversation 
            to respond to; decide which and write an appropriate response.
            \n### Prompt: """

        for message in messages:
            if message["role"] == "user":
                user_message = "\n" + message["content"]
                full_prompt += user_message
            if message["role"] == "assistant":
                assistant_message = "\n### Response: " + message["content"]
                full_prompt += assistant_message

        if default_prompt_footer:
            full_prompt += "\n### Response:"

        return full_prompt


def append_bin_suffix_if_missing(model_name):
    """
    Append ".bin" to model name if not present.

    Args:
        model_name: Name of the model.

    Returns:
        Model name with ".bin" suffix.
    """
    if not model_name.endswith(".bin"):
        model_name += ".bin"
    return model_name
