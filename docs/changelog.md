# Changelog

All notable changes to AI-OS will be documented in this file.

## V0.3.0 — ESP32 联网通信 (2026-07-12)

### Added
- ESP-IDF 项目骨架（esp32/）
- WiFi 连接模块（wifi.c / wifi.h）
  - 事件驱动架构，支持自动重试 5 次
  - SSID/密码通过 menuconfig 配置
- HTTP 客户端模块（server_client.c / server_client.h）
  - GET /health 请求
  - JSON 解析服务器返回状态
- 主程序（main.c）：NVS → WiFi → 服务器通信 → 主循环

### Changed
- 所有代码添加详细中文注释

## V0.2.0 — FastAPI 服务器 (2026-07-12)

### Added
- FastAPI 项目脚手架
- 基础配置体系（pydantic-settings，支持 .env 文件）
- GET /health 健康检查端点
- CORS 中间件
- 单元测试覆盖

### Changed
- 项目结构正式确立

## V0.1.0 — 项目初始化 (2026-07-12)

### Added
- Git 仓库创建
- README 编写
- Roadmap 制定
- Architecture 文档
- ADR 初始化
