/**
 * @file bms_manager.h
 * @brief BMS Manager orchestrator header.
 */

#pragma once

#include "bms_interface.h"
#include "esp_err.h"

/**
 * @brief Initializes the BMS subsystem based on NVS configuration or auto-detection.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t bms_manager_init(void);

/**
 * @brief Reads telemetry data from the active BMS driver.
 * 
 * @param data Pointer to store telemetry readings.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t bms_manager_read(bms_data_t *data);