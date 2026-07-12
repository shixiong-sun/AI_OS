"""
开发环境启动脚本。

直接运行此文件即可启动开发服务器：
    python run.py

生产环境建议使用：
    uvicorn app.main:app --host 0.0.0.0 --port 8000
"""

import uvicorn

from app.config.settings import settings

if __name__ == "__main__":
    """
    启动 UVicron 开发服务器。

    配置说明：
    - host/port：来自 settings，默认 0.0.0.0:8000
    - reload=True：代码修改后自动重启（仅调试模式）
    - log_level：日志级别，可通过环境变量 AIOS_LOG_LEVEL 覆盖

    启动后访问：
    - http://localhost:8000/health   → 健康检查
    - http://localhost:8000/docs     → Swagger API 文档
    - http://localhost:8000/redoc    → ReDoc API 文档
    """
    uvicorn.run(
        "app.main:app",                 # ASGI 应用路径
        host=settings.host,             # 监听地址
        port=settings.port,             # 监听端口
        reload=settings.debug,          # 热重载
        log_level=settings.log_level,   # 日志级别
    )
