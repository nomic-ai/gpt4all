# Monitoring

Use OpenTelemetry to monitor your LLM application in real-time. OpenLIT simplifies the process of gathering insights on interactions, usage metrics, and more, enabling you to optimize the performance and reliability of your GPT4All based LLM application.

## How it works?

OpenLIT extends the capabilities of the gpt4all Python SDK by adding automated data instrumentation. It monitors the `generate` and `embedding` functions, generating OpenTelemetry data (traces and metrics) that help you understand how your application performs under various conditions. This telemetry data can be easily integrated with popular observability platforms like Grafana and DataDog, which allows for in-depth analysis and visualization.

## Getting Started

Here’s a straightforward guide to help you set up and start monitoring your application:

### 1. Install the OpenLIT SDK
Open your terminal and run:

```shell
pip install openlit
```

### 2. Setup Your Application for Monitoring
In your Python script, configure OpenLIT to work with gpt4all:

```python
from gpt4all import GPT4All
import openlit

openlit.init()  # Initialize OpenLIT monitoring

model = GPT4All(model_name='orca-mini-3b-gguf2-q4_0.gguf')

# Start a chat session and send queries
with model.chat_session():
    response1 = model.generate(prompt='hello', temp=0)
    response2 = model.generate(prompt='write me a short poem', temp=0)
    response3 = model.generate(prompt='thank you', temp=0)

    print(model.current_chat_session)
```
This setup wraps your gpt4all model interactions within a monitored session, capturing valuable data about each request and response.

### Visualize

Once you've set up data collection with OpenLIT, you can visualize and analyze this information to better understand your application's performance:

- **Using OpenLIT UI:** Connect to OpenLIT's UI to start exploring performance metrics. Visit the OpenLIT [Quickstart Guide](https://docs.openlit.io/latest/quickstart) for step-by-step details.

- **Integrate with existing Observability Tools:** If you use tools like Grafana or DataDog, you can integrate the data collected by OpenLIT. For instructions on setting up these connections, check the OpenLIT [Connections Guide](https://docs.openlit.io/latest/connections/intro).
