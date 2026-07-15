"""
音频上传接口。

接收 ESP32 采集的 PCM 音频数据，添加 WAV 头部后保存到服务器。

数据流：
  ESP32 录音 -> I2S -> PCM 裸数据 -> HTTP POST -> Server -> WAV 文件

完成标准：
  服务器收到一个真实声音文件。
"""

import logging
import struct
import time
from pathlib import Path

from fastapi import APIRouter, File, HTTPException, Request, UploadFile

router = APIRouter()


@router.post("/audio/raw")
async def upload_audio_raw(request: Request):
    """接收裸 PCM 数据（ESP32 直接 POST 二进制数据）。"""
    raw_data = await request.body()
    if not raw_data:
        raise HTTPException(status_code=400, detail="上传数据为空")

    logging.info(
        "收到裸音频上传: 大小=%d 字节", len(raw_data)
    )

    wav_header = _build_wav_header(len(raw_data))
    wav_data = wav_header + raw_data

    recordings_dir = _ensure_recordings_dir()
    filename = _generate_filename()
    filepath = recordings_dir / filename

    try:
        filepath.write_bytes(wav_data)
    except OSError as e:
        raise HTTPException(status_code=500, detail=f"写入文件失败: {e}")

    duration_sec = len(raw_data) / (
        WAV_SAMPLE_RATE * WAV_CHANNELS * (WAV_BITS_PER_SAMPLE // 8)
    )

    logging.info("音频已保存: %s (%.1f 秒)", filename, duration_sec)

    return {
        "status": "ok",
        "filename": filename,
        "size": len(raw_data),
        "duration": round(duration_sec, 1),
    }



RECORDINGS_DIR = Path(__file__).resolve().parent.parent.parent / "recordings"

WAV_SAMPLE_RATE = 16000
WAV_BITS_PER_SAMPLE = 16
WAV_CHANNELS = 1


def _ensure_recordings_dir() -> Path:
    RECORDINGS_DIR.mkdir(parents=True, exist_ok=True)
    return RECORDINGS_DIR


def _build_wav_header(data_size: int) -> bytes:
    byte_rate = WAV_SAMPLE_RATE * WAV_CHANNELS * (WAV_BITS_PER_SAMPLE // 8)
    block_align = WAV_CHANNELS * (WAV_BITS_PER_SAMPLE // 8)
    riff_chunk_size = 4 + 24 + 8 + data_size

    header = struct.pack(
        "<4sI4s" "4sI" "HHIIHH" "4sI",
        b"RIFF",
        riff_chunk_size,
        b"WAVE",
        b"fmt ",
        16,
        1,  # PCM format
        WAV_CHANNELS,
        WAV_SAMPLE_RATE,
        byte_rate,
        block_align,
        WAV_BITS_PER_SAMPLE,
        b"data",
        data_size,
    )
    return header


def _generate_filename() -> str:
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    return f"recording_{timestamp}.wav"


@router.post("/audio")
async def upload_audio(file: UploadFile = File(...)):
    """接收 PCM 音频数据，保存为 WAV 文件。"""
    raw_data = await file.read()
    if not raw_data:
        raise HTTPException(status_code=400, detail="上传数据为空")

    logging.info(
        "收到音频上传: %s, 大小=%d 字节",
        file.filename or "unknown",
        len(raw_data),
    )

    wav_header = _build_wav_header(len(raw_data))
    wav_data = wav_header + raw_data

    recordings_dir = _ensure_recordings_dir()
    filename = _generate_filename()
    filepath = recordings_dir / filename

    try:
        filepath.write_bytes(wav_data)
    except OSError as e:
        raise HTTPException(status_code=500, detail=f"写入文件失败: {e}")

    duration_sec = len(raw_data) / (
        WAV_SAMPLE_RATE * WAV_CHANNELS * (WAV_BITS_PER_SAMPLE // 8)
    )

    logging.info("音频已保存: %s (%.1f 秒)", filename, duration_sec)

    return {
        "status": "ok",
        "filename": filename,
        "size": len(raw_data),
        "duration": round(duration_sec, 1),
    }
