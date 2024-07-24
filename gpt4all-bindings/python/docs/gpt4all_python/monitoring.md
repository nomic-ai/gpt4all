# GPT4All Monitoring

GPT4All integrates with [OpenLIT](https://github.com/openlit/openlit) OpenTelemetry auto-instrumentation to perform real-time monitoring of your LLM application and GPU hardware.

Monitoring can enhance your GPT4All deployment with auto-generated traces and metrics for

- **Performance Optimization:** Analyze latency, cost and token usage to ensure your LLM application runs efficiently, identifying and resolving performance bottlenecks swiftly.
  
- **User Interaction Insights:** Capture each prompt and response to understand user behavior and usage patterns better, improving user experience and engagement.
  
- **Detailed GPU Metrics:** Monitor essential GPU parameters such as utilization, memory consumption, temperature, and power usage to maintain optimal hardware performance and avert potential issues.

## Setup Monitoring

!!! note "Setup Monitoring"

    With [OpenLIT](https://github.com/openlit/openlit), you can automatically monitor traces and metrics for your LLM deployment:

    ```shell
    pip install openlit
    ```

    ```python
    from gpt4all import GPT4All
    import openlit

    openlit.init()  # start
    # openlit.init(collect_gpu_stats=True)  # Optional: To configure GPU monitoring

    model = GPT4All(model_name='orca-mini-3b-gguf2-q4_0.gguf')

    # Start a chat session and send queries
    with model.chat_session():
        response1 = model.generate(prompt='hello', temp=0)
        response2 = model.generate(prompt='write me a short poem', temp=0)
        response3 = model.generate(prompt='thank you', temp=0)

        print(model.current_chat_session)
    ```

## Visualization

### OpenLIT UI

Connect to OpenLIT's UI to start exploring the collected LLM performance metrics and traces. Visit the OpenLIT [Quickstart Guide](https://docs.openlit.io/latest/quickstart) for step-by-step details.

### Grafana, DataDog, & Other Integrations

You can also send the data collected by OpenLIT to popular monitoring tools like Grafana and DataDog. For detailed instructions on setting up these connections, please refer to the OpenLIT [Connections Guide](https://docs.openlit.io/latest/connections/intro).
