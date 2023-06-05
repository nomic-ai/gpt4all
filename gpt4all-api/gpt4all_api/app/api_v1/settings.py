from pydantic import BaseSettings


class Settings(BaseSettings):
    app_environment = 'dev'
    gpt4all_model: str = 'ggml-gpt4all-j-v1.3-groovy'


settings = Settings()
