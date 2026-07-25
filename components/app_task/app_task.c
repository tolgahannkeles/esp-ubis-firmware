/**
 * @file app_task.c
 * @brief Implementation of the main application worker task (BMS polling & MQTT publishing orchestrator).
 * @details Handles the background worker loop, fetches telemetry safely from the BMS driver layer,
 *          and dispatches packages through the secure MQTT client manager.
 */

#include "app_task.h"
#include "bms_manager.h"
#include "mqtt_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/// Logging tag for Application Worker module
static const char *TAG = "APP_TASK";

/**
 * @brief Main background worker routine handling periodic BMS reads and MQTT dispatching safely.
 * 
 * @param pvParameters Task parameters (unused).
 */
static void app_worker_task_routine(void *pvParameters)
{
    (void)pvParameters;
    bms_data_t bms_data;

    ESP_LOGI(TAG, "Application background worker task started successfully.");

    while (1) {
        // Initialize structure to prevent garbage values or lingering states
        memset(&bms_data, 0, sizeof(bms_data_t));

        // 1. Fetch latest raw telemetry from BMS driver layer safely
        if (bms_manager_read(&bms_data) == ESP_OK && bms_data.is_online) {
            ESP_LOGI(TAG, "Telemetry Report -> V: %.2fV | I: %.2fA | SoC: %u%% | T: %.1f°C",
                     bms_data.voltage, bms_data.current, bms_data.soc, bms_data.temperature);

            // 2. Dispatch telemetry via MQTT if secure connection is active
            if (mqtt_manager_is_connected()) {
                esp_err_t pub_err = mqtt_manager_publish_telemetry(
                    bms_data.voltage, 
                    bms_data.current, 
                    bms_data.soc, 
                    bms_data.temperature
                );
                if (pub_err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to publish telemetry package (err: %d)", pub_err);
                }
            } else {
                ESP_LOGD(TAG, "MQTT client disconnected. Skipping telemetry dispatch.");
            }
        } else {
            ESP_LOGW(TAG, "BMS is offline or failed to read telemetry data.");
        }

        // Periodic loop interval (2 seconds)
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Initializes and spawns the main application background worker task.
 * 
 * @return esp_err_t ESP_OK on success, or ESP_ERR_NO_MEM if task creation fails.
 */
esp_err_t app_task_init(void)
{
    ESP_LOGI(TAG, "Initializing application worker task...");

    BaseType_t task_created = xTaskCreate(
        app_worker_task_routine, 
        "app_task", 
        4096, 
        NULL, 
        5, 
        NULL
    );

    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "CRITICAL: Failed to create application worker task!");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Application worker task successfully created and scheduled.");
    return ESP_OK;
}