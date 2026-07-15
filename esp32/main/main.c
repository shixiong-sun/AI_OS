/**
 * @file main.c
 * @brief AI-OS ESP32 主程序
 *
 * Sprint 4 目标：ESP32 音频采集（INMP441 + I2S）
 *   1. 连接 WiFi（凭据从 menuconfig 读取）
 *   2. 发送 HTTP GET 到 AI-OS 服务器 /health 接口
 *   3. 打印服务器响应状态和版本
 *
 * 程序流程：
 *   NVS 初始化 → WiFi 连接 → 服务器通信 → I2S录音 → HTTP上传 → 循环
 *
 * 如果 WiFi 或服务器连接失败，程序会等待 10 秒后自动重启，
 * 这是一个简单的容错机制，后续 Sprint 会改成更优雅的重试。
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"
#include "server_client.h"
#include "audio.h"

/* ── 日志标签 ────────────────────────────────────────── */
static const char *TAG = "aios_main";


/**
 * @brief ESP32 应用程序入口
 *
 * ESP-IDF 的入口函数，由 FreeRTOS 在启动后自动调用。
 * 相当于普通 C 程序的 main() 函数。
 *
 * 执行流程：
 *   1. 初始化 NVS（非易失性存储），用于保存 WiFi 配置等
 *   2. 连接 WiFi 热点
 *   3. 测试与 AI-OS 服务器的连通性
 *   4. 进入无限循环，保持程序运行
 *
 * 注意：ESP-IDF 要求 app_main 不能返回。
 */
void app_main(void)
{
    /* ── 第一步：初始化 NVS 闪存存储 ──────────────────── */
    /*
     * NVS（Non-Volatile Storage）是 ESP32 的持久化存储。
     * WiFi 驱动内部使用 NVS 保存配置。
     * 如果 NVS 损坏或版本不匹配，需要擦除后重新初始化。
     */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS 需要擦除才能继续 */
        ESP_LOGW(TAG, "NVS 需要擦除，正在重新初始化...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);  /* 如果还失败，直接崩溃（方便调试） */
    ESP_LOGI(TAG, "NVS 初始化完成");

    /* ── 第二步：连接 WiFi ─────────────────────────────── */
    /*
     * 使用 menuconfig 中配置的 SSID 和密码。
     * wifi_init_sta() 会阻塞直到连接成功或失败。
     * 失败时会自动重试 5 次。
     */
    ESP_LOGI(TAG, "正在连接 WiFi（SSID: %s）...",
             CONFIG_AIOS_WIFI_SSID);

    ret = wifi_init_sta();
    if (ret != ESP_OK) {
        /*
         * WiFi 连接失败，可能是：
         * - WiFi 名称或密码错误
         * - 热点不在范围内
         * - 路由器故障
         *
         * 简单的容错：等待 10 秒后重启。
         * TODO: 后续 Sprint 改成进入配网模式（SoftAP + Web 配置）
         */
        ESP_LOGE(TAG, "WiFi 连接失败，10 秒后重启...");
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_restart();  /* 重启 ESP32 */
    }

    /* ── 第三步：测试服务器连接 ────────────────────────── */
    /*
     * 向 AI-OS 服务器发送 GET /health 请求。
     * 服务器地址和端口通过 menuconfig 配置。
     *
     * 如果这一步失败，请检查：
     * - 服务器是否已启动（终端运行 python run.py）
     * - ESP32 和服务器是否在同一网络
     * - menuconfig 中的服务器 IP 是否正确
     */
    ret = server_test_connection();

    /* ── 第四步：输出结果 ──────────────────────────────── */
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "===========================================");
        ESP_LOGI(TAG, "  ✅ AI-OS ESP32 上线成功！");
        ESP_LOGI(TAG, "===========================================");
    } else {
        ESP_LOGE(TAG, "❌ 服务器无法访问，请检查网络和服务器状态");
    }

    /* ── 第五步：录音并上传 ────────────────────────────── */
    /*
     * 初始化 I2S 麦克风（INMP441），录制一段音频，
     * 将 PCM 数据上传到 AI-OS 服务器保存为 WAV 文件。
     *
     * 录音参数通过 menuconfig 配置：
     *   CONFIG_AIOS_AUDIO_DURATION_MS — 录音时长
     */
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "正在初始化 I2S 麦克风...");

        ret = audio_init();
        if (ret == ESP_OK) {
            /* 计算缓冲区大小：采样率 * 时长(秒) * 字节数/样本 */
            size_t buf_size = (CONFIG_AIOS_AUDIO_DURATION_MS / 1000)
                              * AUDIO_SAMPLE_RATE * (AUDIO_BITS / 8);
            char *audio_buf = malloc(buf_size);
            if (audio_buf) {
                size_t bytes_read = 0;

                ESP_LOGI(TAG, "开始录音 (%d ms)...",
                         CONFIG_AIOS_AUDIO_DURATION_MS);

                ret = audio_record(audio_buf, buf_size, &bytes_read);
                if (ret == ESP_OK && bytes_read > 0) {
                    ESP_LOGI(TAG, "录音完成: %zu 字节 (%.1f 秒)",
                             bytes_read,
                             (float)bytes_read / (AUDIO_SAMPLE_RATE * 2));

                    ESP_LOGI(TAG, "正在上传音频到服务器...");
                    ret = server_upload_audio(audio_buf, bytes_read);
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "✅ 音频上传成功！");
                    } else {
                        ESP_LOGE(TAG, "❌ 音频上传失败");
                    }
                } else {
                    ESP_LOGE(TAG, "录音失败");
                }

                free(audio_buf);
            } else {
                ESP_LOGE(TAG, "内存不足，无法分配录音缓冲区");
            }

            audio_deinit();
        } else {
            ESP_LOGE(TAG, "I2S 麦克风初始化失败");
        }
    }

    /* ── 第六步：主循环 ────────────────────────────────── */
    /*
     * ESP-IDF 要求 app_main 不能返回。
     * 后续 Sprint 会在这里添加：
     * - 定期录音和发送
     * - 接收 TTS 音频并播放
     * - 主动通知检查
     */
    while (1) {
        /* 每 60 秒唤醒一次，保持运行 */
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
