idf_component_register(
    SRCS "main.c" "wifi.c" "server_client.c" "audio.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi esp_netif nvs_flash esp_http_client cJSON driver
)
