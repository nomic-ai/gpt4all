FROM ubuntu:22.04

COPY ./chat/gpt4all-* /opt/gpt4all/chat/
COPY ./launcher.sh /opt/gpt4all/

WORKDIR /opt/gpt4all

CMD ["/bin/sh", "-c", "./launcher.sh"]
