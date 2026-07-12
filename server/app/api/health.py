"""Health check endpoint."""

from datetime import datetime, timezone

from fastapi import APIRouter

router = APIRouter()


@router.get("/health")
async def health():
    return {
        "status": "ok",
        "service": "ai-os-server",
        "version": "0.2.0",
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }
