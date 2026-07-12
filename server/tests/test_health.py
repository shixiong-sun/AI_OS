"""
健康检查接口的单元测试。

测试策略：
1. 验证 /health 返回 HTTP 200
2. 验证返回的 JSON 包含所有必要字段
3. 验证字段值符合预期格式

运行方式：
    cd server && pytest -v tests/
    或
    cd server && PYTHONPATH=. pytest -v tests/
"""

from httpx import AsyncClient, ASGITransport
import pytest

from app.main import app


@pytest.fixture
def client():
    """
    创建测试用的 HTTP 客户端。

    使用 ASGITransport 直接调用 FastAPI 应用，
    不需要启动真实的 HTTP 服务器，测试更快。
    """
    # ASGITransport 直接在内存中调用 ASGI 应用
    transport = ASGITransport(app=app)
    return AsyncClient(transport=transport, base_url="http://test")


@pytest.mark.asyncio
async def test_health_returns_200(client):
    """测试：健康检查接口应该返回 HTTP 200 状态码。"""
    resp = await client.get("/health")
    assert resp.status_code == 200


@pytest.mark.asyncio
async def test_health_body(client):
    """测试：健康检查接口的返回体应该包含所有必要字段。"""
    resp = await client.get("/health")
    body = resp.json()

    # 验证 status 字段为 "ok"
    assert body["status"] == "ok", "status 应该为 ok"

    # 验证 service 字段正确
    assert body["service"] == "ai-os-server", "service 名称不正确"

    # 验证 timestamp 字段存在
    assert "timestamp" in body, "应该包含时间戳"
