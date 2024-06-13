# Monitoring

Leverage OpenTelemetry to perform real-time monitoring of your LLM application and GPUs using [OpenLIT](https://github.com/openlit/openlit). This tool helps you easily collect data on user interactions, performance metrics, along with GPU Performance metrics, which can assist in enhancing the functionality and dependability of your GPT4All based LLM application.

## How it works?

OpenLIT adds automatic OTel instrumentation to the GPT4All SDK. It covers the `generate` and `embedding` functions, helping to track LLM usage by gathering inputs and outputs. This allows users to monitor and evaluate the performance and behavior of their LLM application in different environments. OpenLIT also provides OTel auto-instrumentation for monitoring GPU metrics like utilization, temperature, power usage, and memory usage.

Additionally, you have the flexibility to view and analyze the generated traces and metrics either in the OpenLIT UI or by exporting them to widely used observability tools like Grafana and DataDog for more comprehensive analysis and visualization.

## Getting Started

Here’s a straightforward guide to help you set up and start monitoring your application:

### 1. Install the OpenLIT SDK
Open your terminal and run:

```shell
pip install openlit
```

### 2. Setup Monitoring for your Application
In your application, initiate OpenLIT as outlined below:

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
This setup wraps your gpt4all model interactions, capturing valuable data about each request and response.

### 3. (Optional) Enable GPU Monitoring

If your application runs on NVIDIA GPUs, you can enable GPU stats collection in the OpenLIT SDK by adding `collect_gpu_stats=True`. This collects GPU metrics like utilization, temperature, power usage, and memory-related performance metrics. The collected metrics are OpenTelemetry gauges.

```python
from gpt4all import GPT4All
import openlit

openlit.init(collect_gpu_stats=True)  # Initialize OpenLIT monitoring

model = GPT4All(model_name='orca-mini-3b-gguf2-q4_0.gguf')

# Start a chat session and send queries
with model.chat_session():
    response1 = model.generate(prompt='hello', temp=0)
    response2 = model.generate(prompt='write me a short poem', temp=0)
    response3 = model.generate(prompt='thank you', temp=0)

    print(model.current_chat_session)
```

### Visualize

Once you've set up data collection with [OpenLIT](https://github.com/openlit/openlit), you can visualize and analyze this information to better understand your application's performance:

- **Using OpenLIT UI:** Connect to OpenLIT's UI to start exploring performance metrics. Visit the OpenLIT [Quickstart Guide](https://docs.openlit.io/latest/quickstart) for step-by-step details.

- **Integrate with existing Observability Tools:** If you use tools like Grafana or DataDog, you can integrate the data collected by OpenLIT. For instructions on setting up these connections, check the OpenLIT [Connections Guide](https://docs.openlit.io/latest/connections/intro).
