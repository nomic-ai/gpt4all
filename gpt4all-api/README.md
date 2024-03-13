# GPT4All REST API

NOTICE: We are considering to deprecate this API as it has become challenging to maintain and test. If you have any interest in maintaining this or would like to takeover and adopt or discuss the future of this API please speak up in the discord channel.

This directory contains the source code to run and build docker images that run a FastAPI app
for serving inference from GPT4All models. The API matches the OpenAI API spec.

## Tutorial

The following tutorial assumes that you have checked out this repo and cd'd into it.

### Starting the app

First change your working directory to `gpt4all/gpt4all-api`.

Now you can build the FastAPI docker image. You only have to do this on initial build or when you add new dependencies to the requirements.txt file:
```bash
DOCKER_BUILDKIT=1 docker build -t gpt4all_api --progress plain -f gpt4all_api/Dockerfile.buildkit .
```

Then, start the backend with:

```bash
docker compose up --build
```

This will run both the API and locally hosted GPU inference server. If you want to run the API without the GPU inference server, you can run:

```bash
docker compose up --build gpt4all_api
```

To run the API with the GPU inference server, you will need to include environment variables (like the `MODEL_ID`). Edit the `.env` file and run
```bash
docker compose --env-file .env up --build
```


#### Spinning up your app
Run `docker compose up` to spin up the backend. Monitor the logs for errors in-case you forgot to set an environment variable above.


#### Development
Run

```bash
docker compose up --build
```
and edit files in the `app` directory. The api will hot-reload on changes.

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


#### Running inference
```python
import openai
openai.api_base = "http://localhost:4891/v1"

openai.api_key = "not needed for a local LLM"


def test_completion():
    model = "gpt4all-j-v1.3-groovy"
    prompt = "Who is Michael Jordan?"
    response = openai.Completion.create(
        model=model,
        prompt=prompt,
        max_tokens=50,
        temperature=0.28,
        top_p=0.95,
        n=1,
        echo=True,
        stream=False
    )
    assert len(response['choices'][0]['text']) > len(prompt)
    print(response)
```
