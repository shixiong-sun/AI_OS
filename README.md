# AI-OS

AI-OS 是一个长期成长型 AI 数字伙伴。

目标不是做聊天机器人。

目标是打造：

- 长期记忆
- 主动成长
- 多终端接入
- 本地 AI
- ESP32 语音终端
- 数字人格

## 核心理念

> 没有跑起来的功能，不算功能。
> 没有真正用起来的 AI，不算 AI。

## Current Status

| | |
|---|---|
| **Version** | V0.3.0 |
| **Status** | ESP32 联网通信完成 |
| | |
| **已完成** | |
| V0.1 | 项目初始化：Git 仓库、文档体系、ADR |
| V0.2 | FastAPI 服务器：健康检查端点、配置管理、单元测试 |
| V0.3 | ESP32 联网通信：WiFi 连接、HTTP 客户端、JSON 解析 |
| | |
| **下一阶段** | V0.4 — 麦克风录音（I2S 驱动、音频上传到服务器） |

> 以后半年后回来，你自己也知道状态。

## 第一阶段目标

```
ESP32-S3
   ↓
AI Server (FastAPI)
   ↓
LLM
   ↓
TTS → ESP32 语音播放
```

V1 只做一件事：**ESP32 能和 AI 说话**。

没有 Agent、没有长期记忆、没有主动提醒——全部放到后续版本。

## 项目结构

```
AI-OS/
├── README.md
├── .gitignore
├── docs/
│   ├── roadmap.md
│   ├── architecture.md
│   ├── changelog.md
│   └── adr/
├── esp32/          ← ESP32 固件
├── server/         ← AI Server（FastAPI）
├── scripts/        ← 工具脚本
└── .git/
```

## 版本概览

| 版本 | 状态 |
|------|------|
| V0.1 | ✅ 项目初始化 |
| V0.2 | ✅ FastAPI 服务器 |
| V0.3 | ✅ ESP32 联网通信 |
| V0.4 | □ 麦克风录音 |
| V0.5 | □ AI 聊天 |
| V0.6 | □ TTS 播放 |
| V1.0 | □ ESP32 AI 语音助手 |

## 开发原则

1. **先跑起来，再优化，最后重构**
2. **一个 Sprint 只做一个目标**
3. **新需求先进 Backlog，不打断当前 Sprint**
4. **每完成一个 Sprint 必须 Git Commit**
5. **只为当前版本设计，为未来预留扩展，不提前实现未来**
