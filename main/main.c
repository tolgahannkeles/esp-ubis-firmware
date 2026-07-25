/**
 * @file main.c
 * @brief Main application entry point for ESP UBIS firmware architecture.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_manager.h"
#include "network_manager.h"
#include "web_server.h"

static const char *TAG = "UBIS_MAIN";

/**
 * @brief Application main execution sequence coordinating system initialization and provisioning lifecycle.
 */
void app_main(void)
{
    // 1. Initialize the Non-Volatile Storage (NVS) flash driver layer
    ESP_ERROR_CHECK(nvs_manager_init());
    ESP_LOGI(TAG, "NVS subsystem successfully initialized.");

    // 2. Initialize network interfaces, LwIP stack, default event loops, and Wi-Fi drivers
    ESP_ERROR_CHECK(network_manager_init());
    ESP_LOGI(TAG, "Network manager driver initialized.");

    // 3. Initialize the smart provisioning portal (checks NVS credentials or starts AP server interface)
    ESP_ERROR_CHECK(web_server_init_portal());

    ESP_LOGI(TAG, "Firmware boot sequence complete. Connect to 'BMS_CONFIG_XXXX' and browse to http://192.168.4.1 if unconfigured.");

    // Main background idle loop keeping task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}