FROM ubuntu:22.04

COPY ./chat/gpt4all-lora-quantized-linux-x86 /opt/gpt4all/
RUN apt update \
  && apt install -y wget \
  && cd /opt/gpt4all \
  && wget https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized.bin

WORKDIR /opt/gpt4all

CMD ["./gpt4all-lora-quantized-linux-x86", "-m", "./gpt4all-lora-quantized.bin"]
