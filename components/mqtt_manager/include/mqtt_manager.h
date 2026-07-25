/**
 * @file mqtt_manager.h
 * @brief MQTT client manager header for secure telemetry publishing and lifecycle management.
 * @details Provides interface definitions for secure TLS MQTT connectivity, client initialization,
 *          telemetry data publishing, and connection state tracking.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes and starts the secure MQTT client service.
 * @details Reads broker configuration from NVS, sets up TLS parameters with embedded CA certificates,
 *          configures credentials, and registers event callbacks.
 * 
 * @return esp_err_t ESP_OK on success, or appropriate error code on failure.
 */
esp_err_t mqtt_manager_init(void);

/**
 * @brief Stops and cleans up the MQTT client service safely.
 * @details Disconnects from the broker, destroys the client instance, and releases allocated resources.
 * 
 * @return esp_err_t ESP_OK on success, or ESP_FAIL otherwise.
 */
esp_err_t mqtt_manager_stop(void);

/**
 * @brief Publishes BMS telemetry data as a formatted JSON payload to the MQTT broker.
 * @details Performs boundary range validation on parameters and ensures safe transmission with QoS 1.
 * 
 * @param voltage Battery pack voltage in volts (V).
 * @param current Current flow in amperes (A).
 * @param soc State of Charge percentage (0-100 %).
 * @param temperature Battery temperature in degrees Celsius (°C).
 * @return esp_err_t ESP_OK on success, or error code on invalid state/formatting failure.
 */
esp_err_t mqtt_manager_publish_telemetry(float voltage, float current, uint8_t soc, float temperature);

/**
 * @brief Checks whether the MQTT client is currently connected to the broker.
 * 
 * @return true if connected and client handle is active.
 * @return false if disconnected or uninitialized.
 */
bool mqtt_manager_is_connected(void);

#ifdef __cplusplus
}
#endif