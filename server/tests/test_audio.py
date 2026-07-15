"""
音频上传接口的单元测试。

测试策略：
1. 上传合法 PCM 数据 -> 返回 200 + 正确字段
2. 上传空数据 -> 返回 400

运行方式：
    cd server && pytest -v tests/
"""

import struct

from httpx import AsyncClient, ASGITransport
import pytest

from app.main import app


@pytest.fixture
def client():
    transport = ASGITransport(app=app)
    return AsyncClient(transport=transport, base_url="http://test")


def _make_wav_header(data_size):
    """生成合法的 WAV 头部（用于模拟完整的 WAV 文件场景）。"""
    byte_rate = 16000 * 1 * 2
    block_align = 1 * 2
    riff_size = 4 + 24 + 8 + data_size
    return struct.pack(
        "<4sI4s4sIHHIIHH4sI",
        b"RIFF", riff_size, b"WAVE",
        b"fmt ", 16, 1, 1, 16000, byte_rate, block_align, 16,
        b"data", data_size,
    )


@pytest.mark.asyncio
async def test_upload_audio_returns_200(client):
    """测试：上传合法 PCM 数据应该返回 200 及音频信息。"""
    # 生成 1 秒的模拟 PCM 数据（16-bit, 16kHz, mono = 32000 字节）
    pcm_data = b"\x00\x00" * 16000  # 1 秒静音

    resp = await client.post(
        "/audio",
        files={"file": ("test.raw", pcm_data, "application/octet-stream")},
    )
    assert resp.status_code == 200

    body = resp.json()
    assert body["status"] == "ok"
    assert "filename" in body
    assert body["filename"].endswith(".wav")
    assert body["size"] == 32000
    assert body["duration"] == 1.0


@pytest.mark.asyncio
async def test_upload_audio_empty_returns_400(client):
    """测试：上传空数据应该返回 400。"""
    resp = await client.post(
        "/audio",
        files={"file": ("empty.raw", b"", "application/octet-stream")},
    )
    assert resp.status_code == 400
    assert "上传数据为空" in resp.json()["detail"]
