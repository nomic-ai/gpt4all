from pydantic import BaseSettings


class Settings(BaseSettings):
    app_environment = 'dev'
    model: str = 'ggml-mpt-7b-chat.bin'
    gpt4all_path: str = '/models'


settings = Settings()
