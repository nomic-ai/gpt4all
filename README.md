# gpt4all



# Setup

Clone the repo

`git clone --recurse-submodules git@github.com:nomic-ai/gpt4all.git`

Setup the environment

```
python -m pip install -r requirements.txt

cd transformers
pip install -e . 

cd ../peft
pip install -e .
```


## Generate

`python generate.py --config configs/generate/generate.yaml --prompt "Write a script to reverse a string in Python`