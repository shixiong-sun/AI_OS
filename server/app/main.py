"""
AI-OS FastAPI 应用入口。

这是整个 AI 服务器的核心入口文件。
负责：
  1. 创建 FastAPI 应用实例
  2. 注册中间件（CORS 等）
  3. 注册所有路由
  4. 管理应用生命周期（启动/关闭）
"""

import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

# 导入本地模块
from app.config.settings import settings          # 全局配置
from app.api.health import router as health_router  # 健康检查路由
from app.api.audio import router as audio_router    # 音频上传路由


@asynccontextmanager
async def lifespan(app: FastAPI):
    """
    应用生命周期管理。

    yield 之前的代码在应用启动时执行，
    yield 之后的代码在应用关闭时执行。

    当前阶段：
      - 启动时：打印一条日志
      - 后续会在这里初始化：数据库连接、模型加载等

    未来扩展：
      - 连接数据库 / 向量存储
      - 加载 TTS / STT 模型
      - 初始化 Agent 管理器
    """
    logging.info(f"🚀 正在启动 {settings.app_name} v{settings.app_version}")
    yield  # 应用开始处理请求
    logging.info("🛑 正在关闭服务")


# ── 创建 FastAPI 应用 ────────────────────────────────────
app = FastAPI(
    title=settings.app_name,       # API 文档标题
    version=settings.app_version,  # API 版本
    lifespan=lifespan,             # 生命周期管理
)

# ── 注册中间件 ──────────────────────────────────────────
#
# CORS（跨域资源共享）中间件：
# ESP32 作为客户端通过 HTTP 请求，不需要 CORS。
# 但后续加入 Web 端、手机端时，CORS 是必须的。
# 现在提前加上，避免以后改代码。
#
app.add_middleware(
    CORSMiddleware,
    allow_origins=settings.cors_origins,  # 允许的来源域名
    allow_methods=["*"],                   # 允许的 HTTP 方法
    allow_headers=["*"],                   # 允许的请求头
)

# ── 注册路由 ────────────────────────────────────────────
#
# 根路径：AI-OS 入口
# 访问 http://localhost:8000/ 返回项目状态
@app.get("/")
async def root():
    """项目入口，返回 AI-OS 运行状态。"""
    return {
        "project": "AI_OS",
        "status": "running",
    }

#
# 当前路由：
#   GET /        -> 项目入口
#   GET /health  -> 健康检查
#   POST /audio  -> 接收 ESP32 录音
#
# 后续路由：
#   POST /chat   -> 对话接口
#   WS  /ws      -> WebSocket 实时通信
#
app.include_router(health_router)
app.include_router(audio_router)
