"""
Use the OpenAI python API to test gpt4all models.
"""
import openai

openai.api_base = "http://localhost:4891/v1"

openai.api_key = "not needed for a local LLM"


def test_completion():
    model = "gpt4all-j-v1.3-groovy"
    prompt = "Who is Michael Jordan?"
    response = openai.Completion.create(
        model=model, prompt=prompt, max_tokens=50, temperature=0.28, top_p=0.95, n=1, echo=True, stream=False
    )
    assert len(response['choices'][0]['text']) > len(prompt)
    print(response)

def test_streaming_completion():
    model = "gpt4all-j-v1.3-groovy"
    prompt = "Who is Michael Jordan?"
    tokens = []
    for resp in openai.Completion.create(
            model=model,
            prompt=prompt,
            max_tokens=50,
            temperature=0.28,
            top_p=0.95,
            n=1,
            echo=True,
            stream=True):
        tokens.append(resp.choices[0].text)

    assert (len(tokens) > 0)
    assert (len("".join(tokens)) > len(prompt))

# def test_chat_completions():
#     model = "gpt4all-j-v1.3-groovy"
#     prompt = "Who is Michael Jordan?"
#     response = openai.ChatCompletion.create(
#         model=model,
#         messages=[]
#     )
