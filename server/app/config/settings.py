"""
应用配置文件。

通过环境变量加载配置，支持 .env 文件。
所有配置项前缀为 AIOS_，例如 AIOS_DEBUG=true。
"""

from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """应用配置类。

    所有配置项都可以通过环境变量覆盖，例如：
      AIOS_HOST=0.0.0.0  AIOS_PORT=8000  python run.py
    """

    # ── 应用基础信息 ────────────────────────────────────
    app_name: str = "AI-OS Server"          # 应用名称
    app_version: str = "0.2.0"              # 当前版本号
    debug: bool = False                     # 调试模式开关

    # ── 服务器网络配置 ──────────────────────────────────
    host: str = "0.0.0.0"                  # 监听地址（0.0.0.0 = 所有网络接口）
    port: int = 8000                       # 监听端口
    log_level: str = "info"                # 日志级别：debug/info/warning/error

    # ── CORS 跨域配置 ───────────────────────────────────
    # 允许来自哪些域名的请求，* 表示允许所有来源
    cors_origins: list[str] = ["*"]

    # ── LLM 大模型配置（后续 Sprint 使用）───────────────
    openai_api_key: str = ""               # OpenAI API 密钥
    openai_model: str = "gpt-4o-mini"      # 使用的模型名称

    # ── 语音服务配置（后续 Sprint 使用）─────────────────
    stt_engine: str = "openai"             # 语音转文字引擎
    tts_engine: str = "openai"             # 文字转语音引擎

    # pydantic-settings 配置：
    #   env_prefix = "AIOS_"  表示环境变量前缀
    #   env_file = ".env"     表示可以从 .env 文件加载
    model_config = {"env_prefix": "AIOS_", "env_file": ".env"}


# 全局唯一配置实例
# 其他地方通过 from app.config.settings import settings 使用
settings = Settings()
