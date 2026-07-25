/**
 * @file main.c
 * @brief Main application entry point and system orchestration for ESP UBIS firmware.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_manager.h"
#include "network_manager.h"
#include "web_server.h"
#include "bms_manager.h"
#include "mqtt_manager.h"

/// Logging tag for Main Application module
static const char *TAG = "UBIS_MAIN";

/**
 * @brief Periodic background task to poll telemetry data from the active BMS and publish via MQTT.
 * 
 * @param pvParameters Task parameters (unused).
 */
static void bms_polling_task(void *pvParameters)
{
    (void)pvParameters;
    bms_data_t bms_data;

    while (1) {
        // Initialize structure to prevent garbage values
        memset(&bms_data, 0, sizeof(bms_data_t));

        if (bms_manager_read(&bms_data) == ESP_OK && bms_data.is_online) {
            // Log telemetry report periodically or on success
            ESP_LOGI(TAG, "--- BMS Telemetry Report ---");
            ESP_LOGI(TAG, "Voltage     : %.2f V", bms_data.voltage);
            ESP_LOGI(TAG, "Current     : %.2f A", bms_data.current);
            ESP_LOGI(TAG, "Temperature : %.1f °C", bms_data.temperature);
            ESP_LOGI(TAG, "SoC         : %u %%", bms_data.soc);

            // Publish telemetry via MQTT if connected safely
            if (mqtt_manager_is_connected()) {
                esp_err_t pub_err = mqtt_manager_publish_telemetry(bms_data.voltage, bms_data.current, bms_data.soc, bms_data.temperature);
                if (pub_err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to publish telemetry package (err: %d)", pub_err);
                }
            }
        } else {
            ESP_LOGW(TAG, "BMS is offline or failed to read telemetry data.");
        }

        // Polling interval delay (2 seconds)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/**
 * @brief Application entry point. Initializes all system subsystems sequentially and spawns background tasks.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting UBIS BMS Firmware...");

    // 1. Initialize NVS subsystem
    esp_err_t err = nvs_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS subsystem! (err: %d)", err);
    } else {
        ESP_LOGI(TAG, "NVS subsystem successfully initialized.");
    }

    // 2. Initialize Network Manager (Wi-Fi stack & netif)
    err = network_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Network Manager! (err: %d)", err);
    } else {
        ESP_LOGI(TAG, "Network manager driver initialized.");
    }

    // 3. Initialize Web Server / Provisioning Portal & Verify Stored Credentials
    err = web_server_init_portal();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Provisioning portal initialization reported warnings/errors (err: %d)", err);
    }

    // 4. Initialize Secure MQTT Client Manager (TLS + Auth)
    err = mqtt_manager_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "MQTT manager initialization failed or waiting for network link (err: %d)", err);
    }

    // 5. Initialize BMS Manager (Polymorphic driver selection based on NVS)
    if (bms_manager_init() == ESP_OK) {
        ESP_LOGI(TAG, "BMS manager initialized successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to initialize BMS manager!");
    }

    ESP_LOGI(TAG, "Firmware full boot sequence completed successfully.");

    // Create background task for polling BMS telemetry and publishing via MQTT with stack safety check
    BaseType_t task_created = xTaskCreate(bms_polling_task, "bms_polling_task", 4096, NULL, 5, NULL);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "CRITICAL: Failed to create bms_polling_task!");
    }
}