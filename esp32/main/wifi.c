/**
 * @file wifi.c
 * @brief WiFi 连接模块实现
 *
 * 使用 ESP-IDF 的 WiFi station 模式连接路由器。
 * 采用事件驱动架构，通过事件组（EventGroup）同步连接状态。
 *
 * 连接状态机：
 *   STA_START → 开始连接 → DISCONNECTED → 重试 → CONNECTED（成功）
 *                                     ↕
 *                              重试次数 > 5 → 失败
 */

#include "wifi.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

/* ── 日志标签 ────────────────────────────────────────── */
/* 用于区分不同模块的日志输出，在 monitor 中方便过滤 */
static const char *TAG = "aios_wifi";

/* ── 事件组 ───────────────────────────────────────────── */
/* FreeRTOS 事件组，用于在中断上下文和任务间同步 WiFi 状态 */
static EventGroupHandle_t s_wifi_event_group;

/*
 * 事件组比特位定义：
 *   WIFI_CONNECTED_BIT  — 连接成功（收到 IP 地址）
 *   WIFI_FAIL_BIT       — 连接失败（重试耗尽）
 */
#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

/* ── 重试配置 ─────────────────────────────────────────── */
#define MAX_RETRY   5   /* 最大重试次数 */
static int s_retry_num = 0;  /* 当前重试计数 */


/**
 * @brief WiFi 和 IP 事件回调函数
 *
 * 由 ESP-IDF 事件循环在发生 WiFi 或 IP 事件时自动调用。
 * 处理三类事件：
 *   1. WIFI_EVENT_STA_START        — WiFi 初始化完成，开始连接
 *   2. WIFI_EVENT_STA_DISCONNECTED — 连接断开，尝试重连
 *   3. IP_EVENT_STA_GOT_IP         — 获取到 IP 地址，连接成功
 *
 * @param arg          用户自定义参数（本例中未使用）
 * @param event_base   事件基类（WIFI_EVENT 或 IP_EVENT）
 * @param event_id     具体事件 ID
 * @param event_data   事件数据指针
 */
static void event_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        /* WiFi 驱动初始化完成，开始连接配置的热点 */
        esp_wifi_connect();
        ESP_LOGI(TAG, "正在连接 WiFi...");

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        /* 连接丢失或连接失败，尝试重连 */
        if (s_retry_num < MAX_RETRY) {
            /* 还未超过最大重试次数，继续重试 */
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "连接断开，正在重试 (%d/%d)", s_retry_num, MAX_RETRY);
        } else {
            /* 重试次数已耗尽，通知主任务连接失败 */
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "WiFi 连接失败，已重试 %d 次", MAX_RETRY);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        /* 成功获取到 IP 地址，WiFi 连接正式完成 */
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "连接成功！IP 地址: " IPSTR, IP2STR(&event->ip_info.ip));
        /* 重置重试计数，为下次断线重连做准备 */
        s_retry_num = 0;
        /* 通知主任务连接成功 */
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


/**
 * @brief 初始化并连接 WiFi（阻塞版）
 *
 * 这个函数会阻塞直到连接成功或失败。
 * 主要步骤：
 *   1. 创建事件组，用于同步事件回调
 *   2. 初始化 TCP/IP 网络栈
 *   3. 创建默认事件循环
 *   4. 创建 WiFi station 网络接口
 *   5. 初始化 WiFi 驱动
 *   6. 注册事件回调函数
 *   7. 配置 WiFi 参数（SSID、密码、安全模式）
 *   8. 启动 WiFi
 *   9. 等待连接结果（最多等重试 5 次）
 *
 * @return ESP_OK  连接成功
 * @return ESP_FAIL 连接失败
 */
esp_err_t wifi_init_sta(void)
{
    /* 1. 创建事件组，用于事件回调和主任务之间的同步 */
    s_wifi_event_group = xEventGroupCreate();

    /* 2. 初始化 TCP/IP 网络栈 */
    ESP_ERROR_CHECK(esp_netif_init());

    /* 3. 创建默认事件循环，所有系统事件都会发到这里 */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 4. 创建 WiFi station 网络接口 */
    esp_netif_create_default_wifi_sta();

    /* 5. 使用默认配置初始化 WiFi 驱动 */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* 6. 注册事件回调 */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    /* 注册 WiFi 事件处理器（所有 WiFi 事件） */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &event_handler, NULL, &instance_any_id));

    /* 注册 IP 事件处理器（只关心获取到 IP 的事件） */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &event_handler, NULL, &instance_got_ip));

    /* 7. 配置 WiFi 参数 */
    wifi_config_t wifi_config = {
        .sta = {
            /*
             * SSID 和密码从 Kconfig（menuconfig）中读取。
             * 在终端运行 idf.py menuconfig 可以修改这两个值。
             * 默认值定义在 main/Kconfig.projbuild 中。
             */
            .ssid = CONFIG_AIOS_WIFI_SSID,          /* WiFi 名称 */
            .password = CONFIG_AIOS_WIFI_PASSWORD,   /* WiFi 密码 */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, /* 使用 WPA2 加密 */
        },
    };

    /* 8. 设置模式并启动 */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));  /* 设置为 station 模式 */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));  /* 应用配置 */
    ESP_ERROR_CHECK(esp_wifi_start());  /* 启动 WiFi */

    ESP_LOGI(TAG, "WiFi 已启动，等待连接...");

    /* 9. 等待连接结果 */
    /*
     * 阻塞等待两个事件中的任意一个：
     *   WIFI_CONNECTED_BIT — 连接成功
     *   WIFI_FAIL_BIT      — 连接失败
     * portMAX_DELAY 表示无限等待，直到事件发生。
     */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,   /* 不自动清除比特位 */
        pdFALSE,   /* 不需要所有比特位都置位（任一即可） */
        portMAX_DELAY  /* 无限等待 */);

    /* 10. 返回结果 */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi 连接成功！");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "WiFi 连接失败，请检查网络配置");
        return ESP_FAIL;
    }
}
