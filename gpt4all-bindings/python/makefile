SHELL:=/bin/bash -o pipefail
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
PYTHON:=python3

env:
	if [ ! -d $(ROOT_DIR)/env ]; then $(PYTHON) -m venv $(ROOT_DIR)/env; fi

dev: env
	source env/bin/activate; pip install black isort pytest; pip install -e .

documentation:
	rm -rf ./site && mkdocs build

wheel:
	rm -rf dist/ build/ gpt4all/llmodel_DO_NOT_MODIFY; python setup.py bdist_wheel;

clean:
	rm -rf {.pytest_cache,env,gpt4all.egg-info}
	find . | grep -E "(__pycache__|\.pyc|\.pyo$\)" | xargs rm -rf

black:
	source env/bin/activate; black -l 120 -S --target-version py36 gpt4all

isort:
	source env/bin/activate; isort  --ignore-whitespace --atomic -w 120 gpt4all

test:
	source env/bin/activate;  pytest -s gpt4all/tests -k "not test_inference_long"

test_all:
	source env/bin/activate;  pytest -s gpt4all/tests
