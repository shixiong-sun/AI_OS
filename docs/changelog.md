# Changelog

以后每个版本记录在这里。

## V0.1.0 (2026-07-12)

### Added
- 项目骨架与目录结构
- Git 仓库
- README、Roadmap、Architecture 文档
- ADR（架构决策记录）

**Next**: FastAPI 服务器

## V0.2.0 (2026-07-12)

### Added
- FastAPI 项目脚手架与配置体系
- GET /health 健康检查接口
- CORS 中间件
- 单元测试（test_health.py）

**Next**: ESP32 联网通信

## V0.3.0 (2026-07-12)

### Added
- ESP-IDF 项目骨架（esp32/）
- WiFi 连接模块（事件驱动，5 次自动重试）
- HTTP 客户端模块（JSON 解析）
- 主程序：NVS → WiFi → 服务器通信 → 主循环
- 所有代码添加详细中文注释

**Next**: V0.4 — 麦克风录音

## Next

V0.4 — 麦克风录音（I2S 驱动、缓冲区管理、音频上传）
