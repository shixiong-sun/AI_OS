#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

/**
 * @brief Initialize WiFi in station mode, connect to configured AP.
 *
 * Blocks until connected or a fatal error occurs.
 * @return ESP_OK on successful connection, error otherwise.
 */
esp_err_t wifi_init_sta(void);

#endif /* WIFI_H */
