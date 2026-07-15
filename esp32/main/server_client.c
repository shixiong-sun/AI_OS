/**
 * @file server_client.c
 * @brief AI-OS 服务器 HTTP 客户端实现
 *
 * 使用 ESP-IDF 的 esp_http_client 组件与服务器通信。
 * 当前提供连通性测试功能，后续将扩展为完整的通信模块。
 *
 * 使用 cJSON 库解析服务器返回的 JSON 数据。
 * 内存管理注意事项：
 *   - HTTP 响应缓存由事件处理器动态分配和释放
 *   - cJSON 对象使用后必须调用 cJSON_Delete() 释放
 */

#include "server_client.h"
#include <string.h>

#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

/* ── 日志标签 ────────────────────────────────────────── */
static const char *TAG = "aios_client";


/**
 * @brief HTTP 事件回调函数
 *
 * 在 HTTP 请求的各个阶段由 esp_http_client 自动调用。
 * 主要处理 ON_DATA（收到数据）和 ON_FINISH（请求完成）事件。
 *
 * 数据流：
 *   HTTP 响应可能分多个 ON_DATA 事件到达，
 *   我们用动态缓冲区拼接所有数据块，
 *   在 ON_FINISH 时统一解析 JSON。
 */
static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    /* 静态变量，跨多次回调调用保持状态 */
    static char *response_buf = NULL;   /* 响应数据缓冲区 */
    static int response_len = 0;        /* 已接收的数据长度 */

    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        /*
         * 收到 HTTP 响应体数据块。
         * 可能分多次到达，需要拼接到缓冲区中。
         */
        if (!response_buf) {
            /* 第一次收到数据，分配初始缓冲区 */
            response_buf = calloc(1, evt->data_len + 1);
            response_len = 0;
        } else {
            /* 后续数据块，扩展缓冲区 */
            response_buf = realloc(response_buf, response_len + evt->data_len + 1);
        }
        /* 将新数据拷贝到缓冲区末尾 */
        if (response_buf) {
            memcpy(response_buf + response_len, evt->data, evt->data_len);
        }
        response_len += evt->data_len;
        response_buf[response_len] = '\0';  /* 字符串结尾 */
        break;

    case HTTP_EVENT_ON_FINISH:
        /*
         * HTTP 请求完成（成功或失败）。
         * 如果收到了响应数据，尝试解析 JSON。
         */
        if (response_buf) {
            ESP_LOGI(TAG, "服务器响应: %s", response_buf);

            /* 使用 cJSON 解析 JSON 字符串 */
            cJSON *root = cJSON_Parse(response_buf);
            if (root) {
                /* 解析 status 字段 */
                cJSON *status = cJSON_GetObjectItem(root, "status");
                if (status && cJSON_IsString(status)) {
                    ESP_LOGI(TAG, "服务器状态: %s", status->valuestring);
                }

                /* 解析 version 字段 */
                cJSON *version = cJSON_GetObjectItem(root, "version");
                if (version && cJSON_IsString(version)) {
                    ESP_LOGI(TAG, "服务器版本: %s", version->valuestring);
                }

                /* 释放 cJSON 对象，防止内存泄漏 */
                cJSON_Delete(root);
            } else {
                ESP_LOGW(TAG, "JSON 解析失败，服务器可能返回了非 JSON 数据");
            }

            /* 释放响应缓冲区 */
            free(response_buf);
            response_buf = NULL;
            response_len = 0;
        }
        break;

    case HTTP_EVENT_ERROR:
        /* HTTP 请求发生错误（DNS 解析失败、连接超时等） */
        ESP_LOGE(TAG, "HTTP 请求失败");
        break;

    default:
        /* 其他事件（如 HEADERS、DISCONNECTED 等），暂不处理 */
        break;
    }
    return ESP_OK;
}


/**
 * @brief 测试与 AI-OS 服务器的连接
 *
 * 构造 HTTP GET 请求，发送到服务器的 /health 端点，
 * 解析返回的 JSON 并打印服务器状态。
 *
 * 服务器地址格式：http://<host>:<port>/health
 * 其中 host 和 port 通过 menuconfig 配置。
 *
 * @return ESP_OK    成功收到 200 响应
 * @return ESP_FAIL  网络错误或超时
 */
esp_err_t server_test_connection(void)
{
    /* 构造请求 URL */
    char url[128];
    snprintf(url, sizeof(url),
             "http://%s:%d/health",
             CONFIG_AIOS_SERVER_HOST,   /* 服务器 IP，menuconfig 配置 */
             CONFIG_AIOS_SERVER_PORT);  /* 服务器端口，menuconfig 配置 */

    ESP_LOGI(TAG, "正在连接服务器: %s", url);

    /* 配置 HTTP 客户端 */
    esp_http_client_config_t config = {
        .url = url,                     /* 请求地址 */
        .event_handler = _http_event_handler,  /* 事件回调 */
        .timeout_ms = 10000,            /* 超时时间 10 秒 */
        .keep_alive_enable = false,     /* 不启用长连接（简单请求） */
    };

    /* 初始化 HTTP 客户端 */
    esp_http_client_handle_t client = esp_http_client_init(&config);

    /* 执行 HTTP 请求（阻塞，直到请求完成或超时） */
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        /* 请求成功，获取 HTTP 状态码 */
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP 状态码: %d", status_code);

        if (status_code == 200) {
            ESP_LOGI(TAG, "✅ 服务器连接正常！");
        } else {
            ESP_LOGW(TAG, "⚠️  服务器返回非预期状态码: %d", status_code);
        }
    } else {
        /*
         * 请求失败，常见原因：
         *   ESP_ERR_HTTP_CONNECT      — 无法连接（服务器没启动或 IP 不对）
         *   ESP_ERR_HTTP_CONNECTING   — 连接超时
         *   ESP_ERR_HTTP_HANDLE_ERROR — 客户端句柄错误
         */
        ESP_LOGE(TAG, "❌ HTTP 请求失败: %s", esp_err_to_name(err));
    }

    /* 释放 HTTP 客户端资源 */
    esp_http_client_cleanup(client);

    return err;
}


/**
 * @brief HTTP 事件回调函数（音频上传专用）
 *
 * 和 _http_event_handler 结构相同，但专门用于处理音频上传响应。
 * 因为 esp_http_client 每个实例需要独立的事件回调。
 */
static esp_err_t _audio_upload_event_handler(esp_http_client_event_t *evt)
{
    static char *response_buf = NULL;
    static int response_len = 0;

    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (!response_buf) {
            response_buf = calloc(1, evt->data_len + 1);
            response_len = 0;
        } else {
            response_buf = realloc(response_buf, response_len + evt->data_len + 1);
        }
        memcpy(response_buf + response_len, evt->data, evt->data_len);
        response_len += evt->data_len;
        response_buf[response_len] = '\0';
        break;

    case HTTP_EVENT_ON_FINISH:
        if (response_buf) {
            ESP_LOGI(TAG, "服务器响应: %s", response_buf);

            cJSON *root = cJSON_Parse(response_buf);
            if (root) {
                cJSON *status = cJSON_GetObjectItem(root, "status");
                if (status && cJSON_IsString(status)) {
                    ESP_LOGI(TAG, "上传结果: %s", status->valuestring);
                }
                cJSON_Delete(root);
            }

            free(response_buf);
            response_buf = NULL;
            response_len = 0;
        }
        break;

    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "音频上传 HTTP 请求失败");
        break;

    default:
        break;
    }
    return ESP_OK;
}


esp_err_t server_upload_audio(const char *pcm_data, size_t data_size)
{
    if (!pcm_data || data_size == 0) {
        ESP_LOGE(TAG, "音频数据为空");
        return ESP_ERR_INVALID_ARG;
    }

    /* ── 构造请求 URL ────────────────────────────── */
    char url[160];
    snprintf(url, sizeof(url),
             "http://%s:%d/audio/raw",
             CONFIG_AIOS_SERVER_HOST,
             CONFIG_AIOS_SERVER_PORT);

    ESP_LOGI(TAG, "正在上传音频 (%zu 字节) 到 %s", data_size, url);

    /* ── 配置 HTTP POST 请求 ─────────────────────── */
    /*
     * 以裸数据（application/octet-stream）方式发送 PCM 音频。
     * 服务器接收后在服务端添加 WAV 头部。
     *
     * 避免使用 multipart/form-data，ESP32 端构造简单。
     */
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .event_handler = _audio_upload_event_handler,
        .timeout_ms = 30000,
        .buffer_size = 1024,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    /* 设置请求头 */
    esp_http_client_set_header(client, "Content-Type", "application/octet-stream");

    /* 设置请求体（PCM 数据） */
    esp_http_client_set_post_field(client, pcm_data, data_size);

    /* ── 执行请求 ────────────────────────────────── */
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP 状态码: %d", status_code);

        if (status_code == 200) {
            ESP_LOGI(TAG, "音频上传成功！");
        } else {
            ESP_LOGW(TAG, "服务器返回非预期状态码: %d", status_code);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "音频上传失败: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}


/**
 * @brief HTTP 事件回调函数（音频上传专用）
 *
 * 和 _http_event_handler 结构相同，但专门用于处理音频上传响应。
 * 因为 esp_http_client 每个实例需要独立的事件回调。
 */
