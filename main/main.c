/**
 * @file main.c
 * @brief Main application entry point for ESP UBIS firmware.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_manager.h"
#include "network_manager.h"
#include "web_server.h"
#include "bms_manager.h"

static const char *TAG = "UBIS_MAIN";

/**
 * @brief Periodic background task to poll telemetry data from the active BMS.
 */
static void bms_polling_task(void *pvParameters)
{
    bms_data_t bms_data;

    while (1) {
        if (bms_manager_read(&bms_data) == ESP_OK && bms_data.is_online) {
            ESP_LOGI(TAG, "--- BMS Telemetry Report ---");
            ESP_LOGI(TAG, "Voltage     : %.2f V", bms_data.voltage);
            ESP_LOGI(TAG, "Current     : %.2f A", bms_data.current);
            ESP_LOGI(TAG, "Temperature : %.1f °C", bms_data.temperature);
            ESP_LOGI(TAG, "SoC         : %u %%", bms_data.soc);
        } else {
            ESP_LOGW(TAG, "BMS is offline or failed to read telemetry data.");
        }

        // Poll every 2 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    // 1. Initialize NVS subsystem
    ESP_ERROR_CHECK(nvs_manager_init());
    ESP_LOGI(TAG, "NVS subsystem successfully initialized.");

    // 2. Initialize Network Manager (Wi-Fi driver, netif, event loop)
    ESP_ERROR_CHECK(network_manager_init());
    ESP_LOGI(TAG, "Network manager driver initialized.");

    // 3. Initialize Web Server & Provisioning Portal (Checks NVS or starts AP mode)
    ESP_ERROR_CHECK(web_server_init_portal());

    // 4. Initialize BMS Manager (Selects driver based on NVS configuration)
    ESP_ERROR_CHECK(bms_manager_init());
    ESP_LOGI(TAG, "BMS manager initialized successfully.");

    // 5. Create background task for continuous BMS telemetry polling
    xTaskCreate(bms_polling_task, "bms_polling_task", 4096, NULL, 4, NULL);

    ESP_LOGI(TAG, "Firmware full boot sequence completed successfully.");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}