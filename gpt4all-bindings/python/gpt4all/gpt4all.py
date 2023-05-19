"""
Python only API for running all GPT4All models.
"""
import json
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

    def __init__(self, model_name: str, model_path: str = None, model_type: str = None, allow_download=True):
        """
        Constructor

        Args:
            model_name: Name of GPT4All or custom model. Including ".bin" file extension is optional but encouraged.
            model_path: Path to directory containing model file or, if file does not exist, where to download model.
                Default is None, in which case models will be stored in `~/.cache/gpt4all/`.
            model_type: Model architecture to use - currently, options are 'llama', 'gptj', or 'mpt'. Only required if model
                is custom. Note that these models still must be built from llama.cpp or GPTJ ggml architecture.
                Default is None.
            allow_download: Allow API to download models from gpt4all.io. Default is True. 
        """
        self.model = None

        # Model type provided for when model is custom
        if model_type:
            self.model = GPT4All.get_model_from_type(model_type)
        # Else get model from gpt4all model filenames
        else:
            self.model = GPT4All.get_model_from_name(model_name)

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
        response = requests.get("https://gpt4all.io/models/models.json")
        model_json = json.loads(response.content)
        return model_json

    @staticmethod
    def retrieve_model(model_name: str, model_path: str = None, allow_download: bool = True) -> str:
        """
        Find model file, and if it doesn't exist, download the model.

        Args:
            model_name: Name of model.
            model_path: Path to find model. Default is None in which case path is set to
                ~/.cache/gpt4all/.
            allow_download: Allow API to download model from gpt4all.io. Default is True.

        Returns:
            Model file destination.
        """
        
        model_filename = model_name
        if ".bin" not in model_filename:
            model_filename += ".bin"

        # Validate download directory
        if model_path == None:
            model_path = DEFAULT_MODEL_DIRECTORY
            if not os.path.exists(DEFAULT_MODEL_DIRECTORY):
                try:
                    os.makedirs(DEFAULT_MODEL_DIRECTORY)
                except:
                    raise ValueError("Failed to create model download directory at ~/.cache/gpt4all/. \
                    Please specify download_dir.")
        else:
            model_path = model_path.replace("\\", "\\\\")

        if os.path.exists(model_path):
            model_dest = os.path.join(model_path, model_filename).replace("\\", "\\\\")
            if os.path.exists(model_dest):
                print("Found model file.")
                return model_dest

            # If model file does not exist, download
            elif allow_download: 
                # Make sure valid model filename before attempting download
                model_match = False
                for item in GPT4All.list_models():
                    if model_filename == item["filename"]:
                        model_match = True
                        break
                if not model_match:
                    raise ValueError(f"Model filename not in model list: {model_filename}")
                return GPT4All.download_model(model_filename, model_path)
            else:
                raise ValueError("Failed to retrieve model")
        else:
            raise ValueError("Invalid model directory")
        
    @staticmethod
    def download_model(model_filename: str, model_path: str) -> str:
        """
        Download model from https://gpt4all.io.

        Args:
            model_filename: Filename of model (with .bin extension).
            model_path: Path to download model to.

        Returns:
            Model file destination.
        """

        def get_download_url(model_filename):
            return f"https://gpt4all.io/models/{model_filename}"
    
        # Download model
        download_path = os.path.join(model_path, model_filename).replace("\\", "\\\\")
        download_url = get_download_url(model_filename)

        # TODO: Find good way of safely removing file that got interrupted.
        response = requests.get(download_url, stream=True)
        total_size_in_bytes = int(response.headers.get("content-length", 0))
        block_size = 1048576  # 1 MB
        progress_bar = tqdm(total=total_size_in_bytes, unit="iB", unit_scale=True)
        with open(download_path, "wb") as file:
            for data in response.iter_content(block_size):
                progress_bar.update(len(data))
                file.write(data)
        progress_bar.close()

        # Validate download was successful
        if total_size_in_bytes != 0 and progress_bar.n != total_size_in_bytes:
            raise RuntimeError(
                "An error occurred during download. Downloaded file may not work."
            )
        # Sleep for a little bit so OS can remove file lock
        time.sleep(2)

        print("Model downloaded at: " + download_path)
        return download_path

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
        return self.model.generate(prompt, streaming=streaming, **generate_kwargs)
    
    def chat_completion(self, 
                        messages: List[Dict], 
                        default_prompt_header: bool = True, 
                        default_prompt_footer: bool = True, 
                        verbose: bool = True,
                        streaming: bool = True,
                        **generate_kwargs) -> str:
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

        response = self.model.generate(full_prompt, streaming=streaming, **generate_kwargs)

        if verbose and not streaming:
            print(response)

        response_dict = {
            "model": self.model.model_name,
            "usage": {"prompt_tokens": len(full_prompt), 
                      "completion_tokens": len(response), 
                      "total_tokens" : len(full_prompt) + len(response)},
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
        # Helper method to format messages into prompt.
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

    @staticmethod
    def get_model_from_type(model_type: str) -> pyllmodel.LLModel:
        # This needs to be updated for each new model type
        # TODO: Might be worth converting model_type to enum

        if model_type == "gptj":
            return pyllmodel.GPTJModel()
        elif model_type == "llama":
            return pyllmodel.LlamaModel()
        elif model_type == "mpt":
            return pyllmodel.MPTModel()
        else:
            raise ValueError(f"No corresponding model for model_type: {model_type}")
        
    @staticmethod
    def get_model_from_name(model_name: str) -> pyllmodel.LLModel:
        # This needs to be updated for each new model

        # NOTE: We are doing this preprocessing a lot, maybe there's a better way to organize
        if ".bin" not in model_name:
            model_name += ".bin"

        GPTJ_MODELS = [
            "ggml-gpt4all-j-v1.3-groovy.bin",
            "ggml-gpt4all-j-v1.2-jazzy.bin",
            "ggml-gpt4all-j-v1.1-breezy.bin",
            "ggml-gpt4all-j.bin"
        ]

        LLAMA_MODELS = [
            "ggml-gpt4all-l13b-snoozy.bin",
            "ggml-vicuna-7b-1.1-q4_2.bin",
            "ggml-vicuna-13b-1.1-q4_2.bin",
            "ggml-wizardLM-7B.q4_2.bin",
            "ggml-stable-vicuna-13B.q4_2.bin",
            "ggml-nous-gpt4-vicuna-13b.bin"
        ]

        MPT_MODELS = [
            "ggml-mpt-7b-base.bin",
            "ggml-mpt-7b-chat.bin",
            "ggml-mpt-7b-instruct.bin"
        ]

        if model_name in GPTJ_MODELS:
            return pyllmodel.GPTJModel()
        elif model_name in LLAMA_MODELS:
            return pyllmodel.LlamaModel()
        elif model_name in MPT_MODELS:
            return pyllmodel.MPTModel()
        else:
            err_msg = f"""No corresponding model for provided filename {model_name}.
            If this is a custom model, make sure to specify a valid model_type.
            """
            raise ValueError(err_msg)
