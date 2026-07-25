/**
 * @file web_server.h
 * @brief Web server component header for ESP UBIS configuration portal and provisioning manager.
 * @details Provides interface definitions for starting the provisioning web server,
 *          validating NVS stored credentials, and managing the HTTP server lifecycle.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the provisioning manager. 
 * @details Checks NVS for saved credentials. If valid credentials exist, attempts to connect in STA mode 
 *          and verifies MQTT reachability. If credentials are missing, Wi-Fi fails, or MQTT is unreachable, 
 *          falls back to AP mode and launches the configuration web server.
 * 
 * @return esp_err_t ESP_OK on success, or appropriate error code on failure.
 */
esp_err_t web_server_init_portal(void);

/**
 * @brief Stops and destroys the HTTP configuration web server.
 * @details Safely stops the active HTTP daemon instance and releases associated resources.
 * 
 * @return esp_err_t ESP_OK on success, or ESP_FAIL otherwise.
 */
esp_err_t web_server_stop(void);

#ifdef __cplusplus
}
#endif