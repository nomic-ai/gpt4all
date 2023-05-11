SHELL:=/bin/bash -o pipefail
ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
PYTHON:=python3

venv:
	if [ ! -d $(ROOT_DIR)/env ]; then $(PYTHON) -m venv $(ROOT_DIR)/env; fi

documentation:
	rm -rf ./site && mkdocs build

wheel:
	rm -rf dist/ build/ gpt4all/llmodel_DO_NOT_MODIFY; python setup.py bdist_wheel;

clean:
	rm -rf {.pytest_cache,env,gpt4all.egg-info}
	find . | grep -E "(__pycache__|\.pyc|\.pyo$\)" | xargs rm -rf