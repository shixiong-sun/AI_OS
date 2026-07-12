/**
 * @file wifi.h
 * @brief WiFi 连接模块头文件
 *
 * 提供 ESP32 连接 WiFi 的接口。
 * 使用 ESP-IDF 的事件驱动框架，支持自动重连。
 *
 * 使用示例：
 * @code
 *     esp_err_t ret = wifi_init_sta();
 *     if (ret != ESP_OK) {
 *         ESP_LOGE(TAG, "WiFi 连接失败");
 *     }
 * @endcode
 */

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

/**
 * @brief 初始化 WiFi 并连接热点
 *
 * 执行流程：
 * 1. 初始化网络接口（esp_netif）
 * 2. 创建默认事件循环
 * 3. 创建 WiFi station 接口
 * 4. 初始化 WiFi 驱动
 * 5. 注册事件回调（连接成功/失败/重试）
 * 6. 配置 SSID 和密码（从 menuconfig 读取）
 * 7. 启动 WiFi 连接
 * 8. 阻塞等待连接结果
 *
 * 连接失败时会自动重试 5 次（定义在 wifi.c 的 MAX_RETRY）。
 * 重试全部失败后返回 ESP_FAIL。
 *
 * @return ESP_OK  连接成功
 * @return ESP_FAIL 连接失败（重试耗尽或网络不可用）
 */
esp_err_t wifi_init_sta(void);

#endif /* WIFI_H */
