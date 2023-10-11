import sys
from io import StringIO
from pathlib import Path

from gpt4all import GPT4All, Embed4All
import time
import pytest


def test_plugin_instructions():
    model = GPT4All(
        model_name='GPT4All-13B-snoozy.ggmlv3.q4_1.bin',
        plugins=('https://chatgpt-plugins.replit.app/openapi/weather-plugin',)
    )

    print(model.plugin_instructions)


def test_plugin_response():
    model = GPT4All(
        model_name='GPT4All-13B-snoozy.ggmlv3.q4_1.bin',
        plugins=('https://chatgpt-plugins.replit.app/openapi/weather-plugin',)
    )

    output = model.generate(f"""### System:
{model.plugin_instructions}

### Human:
What's the weather in Tokyo? Use the weather plugin.

### Assistant:
""")

    plugin_response = model.get_plugin_response(output)

    print(plugin_response)


def test_plugin():
    model = GPT4All(
        model_name='GPT4All-13B-snoozy.ggmlv3.q4_1.bin',
        plugins=('https://chatgpt-plugins.replit.app/openapi/weather-plugin',)
    )

    output = model.generate(f"""### System:
{model.plugin_instructions}

### Human:
What's the weather in Tokyo? Use the weather plugin.

### Assistant:
""")

    plugin_response = model.get_plugin_response(output)

    output = model.generate(f"""### System:
{plugin_response}

### Human:
What's the weather in Tokyo? Use the weather plugin.

### Assistant:
""")

    print(output)
