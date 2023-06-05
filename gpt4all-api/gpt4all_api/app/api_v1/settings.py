from pydantic import BaseSettings


class Settings(BaseSettings):
    app_environment = 'dev'
    model: str = 'ggml-gpt4all-j-v1.3-groovy'
    gpt4all_path: str = '/models'


settings = Settings()
