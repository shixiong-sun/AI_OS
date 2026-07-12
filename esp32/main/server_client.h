/**
 * @file server_client.h
 * @brief AI-OS 服务器 HTTP 客户端头文件
 *
 * 提供 ESP32 与 AI-OS 后端服务器通信的接口。
 * 当前 Sprint 实现了简单的 GET /health 连通性测试，
 * 后续版本会增加音频上传、对话请求等功能。
 *
 * 使用示例：
 * @code
 *     esp_err_t ret = server_test_connection();
 *     if (ret == ESP_OK) {
 *         ESP_LOGI(TAG, "服务器连接正常！");
 *     }
 * @endcode
 */

#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include "esp_err.h"

/**
 * @brief 测试与 AI-OS 服务器的连接
 *
 * 向服务器发送 HTTP GET /health 请求，验证：
 * 1. 网络通路是否正常（ESP32 → 路由器 → 服务器）
 * 2. 服务器是否正在运行
 * 3. 服务器返回的 JSON 中包含 status: "ok"
 *
 * 服务器地址和端口通过 menuconfig 配置：
 *   CONFIG_AIOS_SERVER_HOST — 服务器 IP 地址
 *   CONFIG_AIOS_SERVER_PORT — 服务器端口
 *
 * @return ESP_OK    服务器响应正常（HTTP 200）
 * @return ESP_FAIL  网络错误或服务器无响应
 */
esp_err_t server_test_connection(void);

#endif /* SERVER_CLIENT_H */
