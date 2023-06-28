# WIP - not yet ready.

# GPT4All REST API
This directory contains the source code to run and build docker images that run a FastAPI app
for serving inference from GPT4All models. The API matches the OpenAI API spec.

## Tutorial

### Starting the app

First build the FastAPI docker image. You only have to do this on initial build or when you add new dependencies to the requirements.txt file:
```bash
DOCKER_BUILDKIT=1 docker build -t gpt4all_api --progress plain -f gpt4all_api/Dockerfile.buildkit .
```

Then, start the backend with:

```bash
docker compose up --build
```

#### Spinning up your app
Run `docker compose up` to spin up the backend. Monitor the logs for errors in-case you forgot to set an environment variable above.


#### Development
Run

```bash
docker compose up --build
```
and edit files in the `api` directory. The api will hot-reload on changes.

You can run the unit tests with

```bash
make test
```

#### Viewing API documentation

Once the FastAPI ap is started you can access its documentation and test the search endpoint by going to:
```
localhost:80/docs
```

This documentation should match the OpenAI OpenAPI spec located at https://github.com/openai/openai-openapi/blob/master/openapi.yaml