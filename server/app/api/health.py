"""
健康检查接口。

提供 /health 端点，用于监控和连通性测试。
ESP32 设备连接服务器时首先会请求这个接口。
"""

from datetime import datetime, timezone

from fastapi import APIRouter

# 创建路由器实例，后续可以在这里添加更多公共端点
router = APIRouter()


@router.get("/health")
async def health():
    """
    健康检查接口。

    返回服务器当前运行状态，包括：
    - status：服务状态（"ok" 表示正常）
    - service：服务名称
    - version：当前版本号
    - timestamp：当前 UTC 时间戳

    这个接口被 ESP32 固件用来验证：
    1. 服务器是否正常运行
    2. 网络连接是否通畅
    3. 返回的版本号用于兼容性检查
    """
    return {
        "status": "ok",                              # 服务正常
        "service": "ai-os-server",                   # 服务标识
        "version": "0.2.0",                          # 当前版本
        "timestamp": datetime.now(timezone.utc).isoformat(),  # UTC 时间
    }
