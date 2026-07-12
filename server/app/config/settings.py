"""Application configuration via environment variables."""

from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    app_name: str = "AI-OS Server"
    app_version: str = "0.2.0"
    debug: bool = False
    host: str = "0.0.0.0"
    port: int = 8000
    log_level: str = "info"

    # CORS
    cors_origins: list[str] = ["*"]

    # LLM (future)
    openai_api_key: str = ""
    openai_model: str = "gpt-4o-mini"

    # STT / TTS (future)
    stt_engine: str = "openai"
    tts_engine: str = "openai"

    model_config = {"env_prefix": "AIOS_", "env_file": ".env"}


settings = Settings()
