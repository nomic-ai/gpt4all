from pydantic import BaseSettings


class Settings(BaseSettings):
    app_environment = 'dev'
    model: str = 'ggml-mpt-7b-chat.bin'
    gpt4all_path: str = '/models'
    inference_mode: str = "cpu"
    hf_inference_server_host: str = "http://gpt4all_gpu:80/generate"


settings = Settings()
