#include "server_client.h"
#include <string.h>

#include "esp_log.h"
#include "esp_http_client.h"
#include "cJSON.h"

static const char *TAG = "aios_client";

static esp_err_t _http_event_handler(esp_http_client_event_t *evt)
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
            ESP_LOGI(TAG, "Server response: %s", response_buf);
            /* Parse JSON */
            cJSON *root = cJSON_Parse(response_buf);
            if (root) {
                cJSON *status = cJSON_GetObjectItem(root, "status");
                if (status && cJSON_IsString(status)) {
                    ESP_LOGI(TAG, "Server status: %s", status->valuestring);
                }
                cJSON *version = cJSON_GetObjectItem(root, "version");
                if (version && cJSON_IsString(version)) {
                    ESP_LOGI(TAG, "Server version: %s", version->valuestring);
                }
                cJSON_Delete(root);
            }
            free(response_buf);
            response_buf = NULL;
            response_len = 0;
        }
        break;

    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP request failed");
        break;

    default:
        break;
    }
    return ESP_OK;
}

esp_err_t server_test_connection(void)
{
    char url[128];
    snprintf(url, sizeof(url),
             "http://%s:%d/health",
             CONFIG_AIOS_SERVER_HOST,
             CONFIG_AIOS_SERVER_PORT);

    ESP_LOGI(TAG, "Connecting to server: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = _http_event_handler,
        .timeout_ms = 10000,
        .keep_alive_enable = false,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP status: %d", status_code);
        if (status_code == 200) {
            ESP_LOGI(TAG, "Server connection OK!");
        } else {
            ESP_LOGW(TAG, "Unexpected status: %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err;
}
