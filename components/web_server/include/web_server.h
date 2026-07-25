/**
 * @file web_server.h
 * @brief Web server component header for ESP UBIS configuration portal and provisioning manager.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initializes the provisioning manager. 
 * 
 * Checks NVS for saved credentials. If valid credentials exist, attempts to connect in STA mode.
 * If credentials are missing or connection fails, starts AP mode and the configuration web server.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t web_server_init_portal(void);

/**
 * @brief Stops and destroys the HTTP configuration web server.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t web_server_stop(void);