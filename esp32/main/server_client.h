/**
 * @file server_client.h
 * @brief AI-OS 服务器 HTTP 客户端头文件
 *
 * 提供 ESP32 与 AI-OS 后端服务器通信的接口。
 *
 * 当前功能：
 *   1. GET /health     - 连通性测试
 *   2. POST /audio/raw - 上传 PCM 音频数据
 */

#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include "esp_err.h"

/**
 * @brief 测试与 AI-OS 服务器的连接
 *
 * 向服务器发送 HTTP GET /health 请求，验证：
 * 1. 网络通路是否正常（ESP32 -> 路由器 -> 服务器）
 * 2. 服务器是否正在运行
 * 3. 服务器返回的 JSON 中包含 status: "ok"
 *
 * 服务器地址和端口通过 menuconfig 配置：
 *   CONFIG_AIOS_SERVER_HOST - 服务器 IP 地址
 *   CONFIG_AIOS_SERVER_PORT - 服务器端口
 *
 * @return ESP_OK    服务器响应正常（HTTP 200）
 * @return ESP_FAIL  网络错误或服务器无响应
 */
esp_err_t server_test_connection(void);

/**
 * @brief 上传 PCM 音频数据到服务器
 *
 * 将录音得到的 PCM 数据通过 HTTP POST 发送到服务器，
 * 服务器会添加 WAV 头部并保存为文件。
 *
 * 发送格式：
 *   POST /audio/raw
 *   Content-Type: application/octet-stream
 *   Body: PCM 裸数据（16-bit, 16kHz, 单声道小端序）
 *
 * @param pcm_data  PCM 音频数据指针
 * @param data_size 数据大小（字节）
 *
 * @return ESP_OK    上传成功
 * @return ESP_FAIL  网络错误或服务器返回非 200
 */
esp_err_t server_upload_audio(const char *pcm_data, size_t data_size);

#endif /* SERVER_CLIENT_H */
