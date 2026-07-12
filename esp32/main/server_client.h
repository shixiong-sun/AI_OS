#ifndef SERVER_CLIENT_H
#define SERVER_CLIENT_H

#include "esp_err.h"

/**
 * @brief Test connection to AI-OS server.
 *
 * Makes GET /health and prints the response.
 * @return ESP_OK if server responded with status ok.
 */
esp_err_t server_test_connection(void);

#endif /* SERVER_CLIENT_H */
