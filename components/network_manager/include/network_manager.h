/**
 * @file network_manager.h
 * @brief Network manager component header for ESP UBIS.
 *        Declares Wi-Fi initialization, AP mode lifecycle, and connection testing routines.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initializes the underlying network interfaces, system event loop, and Wi-Fi driver.
 * 
 * @return 
 *      - ESP_OK on success
 *      - esp_err_t error code on failure
 */
esp_err_t network_manager_init(void);

/**
 * @brief Configures and starts the ESP32 in Access Point (AP) mode.
 * 
 * @return 
 *      - ESP_OK on success
 *      - esp_err_t error code on failure
 */
esp_err_t network_manager_start_ap(void);

/**
 * @brief Stops active AP operations and tears down the access point without rebooting.
 * 
 * @return 
 *      - ESP_OK on success
 *      - esp_err_t error code on failure
 */
esp_err_t network_manager_stop_ap(void);

/**
 * @brief Synchronously tests connection to a specified Wi-Fi router.
 * 
 * @param ssid Target router SSID string.
 * @param password Target router password string.
 * @return 
 *      - true if an IP address is acquired within timeout
 *      - false if connection fails or times out
 */
bool network_manager_test_sta_connection(const char* ssid, const char* password);