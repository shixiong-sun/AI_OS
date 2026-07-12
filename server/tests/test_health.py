"""Tests for the health check endpoint."""

from httpx import AsyncClient, ASGITransport
import pytest

from app.main import app


@pytest.fixture
def client():
    transport = ASGITransport(app=app)
    return AsyncClient(transport=transport, base_url="http://test")


@pytest.mark.asyncio
async def test_health_returns_200(client):
    resp = await client.get("/health")
    assert resp.status_code == 200


@pytest.mark.asyncio
async def test_health_body(client):
    resp = await client.get("/health")
    body = resp.json()
    assert body["status"] == "ok"
    assert body["service"] == "ai-os-server"
    assert "timestamp" in body
