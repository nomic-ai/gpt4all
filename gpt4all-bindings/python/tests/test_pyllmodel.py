from io import StringIO
import sys

from gpt4all import pyllmodel

# TODO: Integration test for loadmodel and prompt. 
# # Right now, too slow b/c it requires file download.

def test_create_gptj():
    gptj = pyllmodel.GPTJModel()
    assert gptj.model_type == "gptj"

def test_create_llama():
    llama = pyllmodel.LlamaModel()
    assert llama.model_type == "llama"

def test_create_mpt():
    mpt = pyllmodel.MPTModel()
    assert mpt.model_type == "mpt"

def prompt_unloaded_mpt():
    mpt = pyllmodel.MPTModel()
    old_stdout = sys.stdout 
    collect_response = StringIO()
    sys.stdout = collect_response

    mpt.prompt("hello there")

    response = collect_response.getvalue()
    sys.stdout = old_stdout

    response = response.strip()
    assert response == "MPT ERROR: prompt won't work with an unloaded model!"

def prompt_unloaded_gptj():
    gptj = pyllmodel.GPTJModel()
    old_stdout = sys.stdout 
    collect_response = StringIO()
    sys.stdout = collect_response

    gptj.prompt("hello there")

    response = collect_response.getvalue()
    sys.stdout = old_stdout

    response = response.strip()
    assert response == "GPT-J ERROR: prompt won't work with an unloaded model!"

def prompt_unloaded_llama():
    llama = pyllmodel.LlamaModel()
    old_stdout = sys.stdout 
    collect_response = StringIO()
    sys.stdout = collect_response

    llama.prompt("hello there")

    response = collect_response.getvalue()
    sys.stdout = old_stdout

    response = response.strip()
    assert response == "LLAMA ERROR: prompt won't work with an unloaded model!"
