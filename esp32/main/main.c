/**
 * @file main.c
 * @brief AI-OS ESP32 — WiFi connection + server communication test.
 *
 * Sprint 3 goal:
 *   1. Connect to WiFi (credentials from menuconfig)
 *   2. Send HTTP GET to AI-OS server /health
 *   3. Print server response
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"
#include "server_client.h"

static const char *TAG = "aios_main";

void app_main(void)
{
    /* ── 1. Initialize NVS ────────────────────────────── */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase, retrying...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    /* ── 2. Connect to WiFi ───────────────────────────── */
    ESP_LOGI(TAG, "Connecting to WiFi (SSID: %s)...",
             CONFIG_AIOS_WIFI_SSID);

    ret = wifi_init_sta();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connection failed, restarting in 10s...");
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_restart();
    }

    /* ── 3. Test server connection ────────────────────── */
    ret = server_test_connection();

    /* ── 4. Report result ─────────────────────────────── */
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "=== AI-OS ESP32 is online! ===");
    } else {
        ESP_LOGE(TAG, "Server unreachable, check network & server");
    }

    /* Keep running */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
