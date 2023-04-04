FROM ubuntu:22.04

COPY ./chat/gpt4all-* /opt/gpt4all/

# If gpt4all-lora-quantized.bin exists in chat dir, comment out below
RUN apt update \
  && apt install -y wget \
  && cd /opt/gpt4all \
  && wget https://the-eye.eu/public/AI/models/nomic-ai/gpt4all/gpt4all-lora-quantized.bin

WORKDIR /opt/gpt4all

CMD ["./gpt4all-lora-quantized-linux-x86", "-m", "./gpt4all-lora-quantized.bin"]
