# GPT4All

GTP4All is an ecosystem to train and deploy **powerful** and **customized** large language models that run locally on consumer grade CPUs.


## Models

A GPT4All model is a 3GB - 8GB file that you can download and plug into the GPT4All open-source ecosystem software. **Nomic AI** supports and maintains this software ecosystem to enforce quality and security alongside spearheading the effort to allow any person or enterprise to easily train and deploy their own on-edge large language models. 


## Best Practices
GPT4All models are designed to run locally on your own CPU. Large prompts may require longer computation time and
result in worse performance. Giving an instruction to the model will typically produce the best results.

There are two methods to interface with the underlying language model, `chat_completion()` and `generate()`. Chat completion formats a user-provided message dictionary into a prompt template (see API documentation for more details and options). This will usually produce much better results and is the approach we recommend. You may also prompt the model with `generate()` which will just pass the raw input string to the model. 