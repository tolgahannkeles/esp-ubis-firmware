/**
 * @file button_manager.h
 * @brief Button manager header for handling long-press factory reset / provisioning trigger.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the hardware button GPIO and configures the interrupt-driven long-press monitor.
 * 
 * @return esp_err_t ESP_OK on success, or appropriate error code on failure.
 */
esp_err_t button_manager_init(void);

#ifdef __cplusplus
}
#endif