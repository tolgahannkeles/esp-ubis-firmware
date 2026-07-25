/**
 * @file app_task.h
 * @brief Application worker header for background business logic and telemetry dispatching.
 * @details Provides the public interface to initialize and spawn the main application worker task
 *          responsible for periodic BMS telemetry acquisition and MQTT dispatching.
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes and spawns the main application background worker task.
 * @details Creates a FreeRTOS background worker task that continuously polls telemetry 
 *          from the BMS manager layer and securely publishes packages via the MQTT client manager.
 * 
 * @return esp_err_t ESP_OK on success, or appropriate error code (e.g., ESP_ERR_NO_MEM) on failure.
 */
esp_err_t app_task_init(void);

#ifdef __cplusplus
}
#endif