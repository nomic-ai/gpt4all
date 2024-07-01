# GPT4All Monitoring

GPT4All integrates with [OpenLIT](https://github.com/openlit/openlit) open telemetry instrumentation to perform real-time monitoring of your LLM application and hardware.

Monitoring can enhance your GPT4All deployment with auto-generated traces for

- performance metrics

- user interactions

- GPU metrics like utilization, memory, temperature, power usage

## Setup Monitoring

!!! note "Setup Monitoring"

    With [OpenLIT](https://github.com/openlit/openlit), you can automatically monitor metrics for your LLM deployment:

    ```shell
    pip install openlit
    ```

    ```python
    from gpt4all import GPT4All
    import openlit

    openlit.init()  # start
    # openlit.init(collect_gpu_stats=True)  # or, start with optional GPU monitoring

    model = GPT4All(model_name='orca-mini-3b-gguf2-q4_0.gguf')

    # Start a chat session and send queries
    with model.chat_session():
        response1 = model.generate(prompt='hello', temp=0)
        response2 = model.generate(prompt='write me a short poem', temp=0)
        response3 = model.generate(prompt='thank you', temp=0)

        print(model.current_chat_session)
    ```

## OpenLIT UI

Connect to OpenLIT's UI to start exploring performance metrics. Visit the OpenLIT [Quickstart Guide](https://docs.openlit.io/latest/quickstart) for step-by-step details.

## Grafana, DataDog, & Other Integrations

If you use tools like , you can integrate the data collected by OpenLIT. For instructions on setting up these connections, check the OpenLIT [Connections Guide](https://docs.openlit.io/latest/connections/intro).
