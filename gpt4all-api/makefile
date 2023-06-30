ROOT_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
APP_NAME:=gpt4all_api
PYTHON:=python3.8

all: dependencies

fresh: clean dependencies

testenv: clean_testenv test_build
	docker compose up --build

testenv_d: clean_testenv test_build
	docker compose up --build -d

test:
	docker compose exec gpt4all_api pytest -svv --disable-warnings -p no:cacheprovider /app/tests

test_build:
    DOCKER_BUILDKIT=1 docker build -t gpt4all_api --progress plain -f gpt4all_api/Dockerfile.buildkit .

clean_testenv:
	docker compose down -v

fresh_testenv: clean_testenv testenv

venv:
	if [ ! -d $(ROOT_DIR)/env ]; then $(PYTHON) -m venv $(ROOT_DIR)/env; fi

dependencies: venv
	source $(ROOT_DIR)/env/bin/activate; yes w | python -m pip install -r $(ROOT_DIR)/atlas_api/requirements.txt

clean: clean_testenv
	# Remove existing environment
	rm -rf $(ROOT_DIR)/env;
	rm -rf $(ROOT_DIR)/$(APP_NAME)/*.pyc;


