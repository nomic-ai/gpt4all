"""
Python only API for running all GPT4All models.
"""
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
    
    Attribuies:
        model: Pointer to underlying C model.
    """

    def __init__(self, model_name: str, model_path: str = None, model_type: str = None, allow_download = True):
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
        self.model_type = model_type
        self.model = pyllmodel.LLModel()
        # Retrieve model and download if allowed
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
    def retrieve_model(model_name: str, model_path: str = None, allow_download: bool = True,
                       verbose: bool = True) -> str:
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

            return GPT4All.download_model(model_filename, model_path, verbose = verbose, url=url)
        else:
            raise ValueError("Failed to retrieve model")

    @staticmethod
    def download_model(model_filename: str, model_path: str, verbose: bool = True, url: str = None) -> str:
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

        # Validate download was successful
        if total_size_in_bytes != 0 and progress_bar.n != total_size_in_bytes:
            raise RuntimeError(
                "An error occurred during download. Downloaded file may not work."
            )

        # Sleep for a little bit so OS can remove file lock
        time.sleep(2)

        if verbose:
            print("Model downloaded at: ", download_path)
        return download_path

    # TODO: this naming is just confusing now and needs to be deprecated now that we have generator
    # Need to better consolidate all these different model response methods
    def generate(self, prompt: str, streaming: bool = True, **generate_kwargs) -> str:
        """
        Surfaced method of running generate without accessing model object.

        Args:
            prompt: Raw string to be passed to model.
            streaming: True if want output streamed to stdout.
            **generate_kwargs: Optional kwargs to pass to prompt context.
        
        Returns:
            Raw string of generated model response.
        """
        return self.model.prompt_model(prompt, streaming=streaming, **generate_kwargs)

    def generator(self, prompt: str, **generate_kwargs) -> str:
        """
        Surfaced method of running generate without accessing model object.

        Args:
            prompt: Raw string to be passed to model.
            streaming: True if want output streamed to stdout.
            **generate_kwargs: Optional kwargs to pass to prompt context.
        
        Returns:
            Raw string of generated model response.
        """
        return self.model.generator(prompt, **generate_kwargs)

    def chat_completion(self,
                        messages: List[Dict],
                        default_prompt_header: bool = True,
                        default_prompt_footer: bool = True,
                        verbose: bool = True,
                        streaming: bool = True,
                        **generate_kwargs) -> dict:
        """
        Format list of message dictionaries into a prompt and call model
        generate on prompt. Returns a response dictionary with metadata and
        generated content.

        Args:
            messages: List of dictionaries. Each dictionary should have a "role" key
                with value of "system", "assistant", or "user" and a "content" key with a
                string value. Messages are organized such that "system" messages are at top of prompt,
                and "user" and "assistant" messages are displayed in order. Assistant messages get formatted as
                "Response: {content}". 
            default_prompt_header: If True (default), add default prompt header after any system role messages and
                before user/assistant role messages.
            default_prompt_footer: If True (default), add default footer at end of prompt.
            verbose: If True (default), print full prompt and generated response.
            streaming: True if want output streamed to stdout.
            **generate_kwargs: Optional kwargs to pass to prompt context.

        Returns:
            Response dictionary with:  
                "model": name of model.  
                "usage": a dictionary with number of full prompt tokens, number of 
                    generated tokens in response, and total tokens.  
                "choices": List of message dictionary where "content" is generated response and "role" is set
                as "assistant". Right now, only one choice is returned by model.
        """
        full_prompt = self._build_prompt(messages,
                                         default_prompt_header=default_prompt_header,
                                         default_prompt_footer=default_prompt_footer)
        if verbose:
            print(full_prompt)

        response = self.model.prompt_model(full_prompt, streaming=streaming, **generate_kwargs)

        if verbose and not streaming:
            print(response)

        response_dict = {
            "model": self.model.model_name,
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
                      default_prompt_footer=True) -> str:
        """
        Helper method for buildilng a prompt using template from list of messages.

        Args:
            messages:  List of dictionaries. Each dictionary should have a "role" key
                with value of "system", "assistant", or "user" and a "content" key with a
                string value. Messages are organized such that "system" messages are at top of prompt,
                and "user" and "assistant" messages are displayed in order. Assistant messages get formatted as
                "Response: {content}".
            default_prompt_header: If True (default), add default prompt header after any system role messages and
                before user/assistant role messages.
            default_prompt_footer: If True (default), add default footer at end of prompt.
        
        Returns:
            Formatted prompt.
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
    if not model_name.endswith(".bin"):
        model_name += ".bin"
    return model_name
