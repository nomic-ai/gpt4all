import os

from pathlib import Path


def get_model_folder(model_name: str = None) -> Path:
    """Derive model folder preference based on the PYTEST_MODEL_PATH env var.

        Example Linux, which takes into account the chat GUI default folder:

        $ export PYTEST_MODEL_PATH="${HOME}/.local/share/nomic.ai/GPT4All:${HOME}/.cache/gpt4all"

    In the test code or a REPL it should then look something like:

        >>> name = 'orca-mini-3b.ggmlv3.q4_0.bin'
        >>> folder = str(get_model_folder(name))
        >>> model = GPT4All(model_name=name, model_path=folder, allow_download=False)
    """
    try:
        model_path_var = os.environ['PYTEST_MODEL_PATH']
    except:
        return None
    if not model_path_var or str.isspace(model_path_var):
        return None
    # PYTEST_MODEL_PATH is expected to be in the same format as other PATH variables
    # i.e. several can be defined if split by 'os.pathsep':
    model_path_strings = [p for p in model_path_var.split(os.pathsep)]
    model_paths = []
    for path_string in model_path_strings:
        if not path_string or path_string.isspace():  # in case of consecutive 'os.pathsep'
            continue
        path = Path(path_string)
        valid_path = True
        if not path.exists():
            print(f"Warning: '{path}' doesn't exist")
            valid_path = False
        elif not path.is_dir():
            print(f"Warning: '{path}' is not a directory")
            valid_path = False
        if valid_path:
            model_paths.append(path)
    if not model_paths:
        return None
    model_name = str(model_name)
    if not model_name or model_name.isspace() or len(model_paths) == 1:
        # if 'model_name' not given, just pick the highest priority path:
        return model_paths[0]
    # otherwise, look for a model file, in order:
    for path in model_paths:
        if (path / model_name).is_file():
            return path
        if (path / (model_name + '.bin')).is_file():
            return path
    # if not found, highest priority path:
    return model_paths[0]
