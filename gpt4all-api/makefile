ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
APP_NAME:=gpt4all_api
PYTHON:=python3.8
SHELL := /bin/bash

all: dependencies

fresh: clean dependencies

testenv: clean_testenv test_build
	docker compose -f docker-compose.yaml up --build

testenv_gpu: clean_testenv test_build
	docker compose -f docker-compose.yaml -f docker-compose.gpu.yaml up --build

testenv_d: clean_testenv test_build
	docker compose up --build -d

test:
	docker compose exec $(APP_NAME) pytest -svv --disable-warnings -p no:cacheprovider /app/tests

test_build:
    DOCKER_BUILDKIT=1 docker build -t $(APP_NAME) --progress plain -f $(APP_NAME)/Dockerfile.buildkit .

clean_testenv:
	docker compose down -v

fresh_testenv: clean_testenv testenv

venv:
	if [ ! -d $(ROOT_DIR)/env ]; then $(PYTHON) -m venv $(ROOT_DIR)/env; fi

dependencies: venv
	source $(ROOT_DIR)/env/bin/activate; $(PYTHON) -m pip install -r $(ROOT_DIR)/$(APP_NAME)/requirements.txt

clean: clean_testenv
	# Remove existing environment
	rm -rf $(ROOT_DIR)/env;
	rm -rf $(ROOT_DIR)/$(APP_NAME)/*.pyc;


black:
	source $(ROOT_DIR)/env/bin/activate; black -l 120 -S --target-version py38 $(APP_NAME)

isort:
	source $(ROOT_DIR)/env/bin/activate; isort  --ignore-whitespace --atomic -w 120 $(APP_NAME)