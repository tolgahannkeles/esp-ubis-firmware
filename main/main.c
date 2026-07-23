/**
 * @file main.c
 * @brief Main application entry point for the ESP UBIS (Universal Battery Interface System).
 *        Currently demonstrates the initialization and testing of the NVS manager component.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_manager.h"

static const char *TAG = "UBIS_MAIN";

void app_main(void)
{
    // Initialize the Non-Volatile Storage (NVS) subsystem
    ESP_ERROR_CHECK(nvs_manager_init());
    ESP_LOGI(TAG, "NVS Hardware is ready.");

    // Retrieve the boot count from the "system" namespace.
    // If the key does not exist (e.g., first boot), it defaults to 0.
    int32_t boot_count = nvs_manager_read_i32("system", "boot_count", 0);
    ESP_LOGI(TAG, "Device is booting for the %ld. time.", boot_count);
    
    // Increment the boot counter and persist it back to NVS
    nvs_manager_write_i32("system", "boot_count", boot_count + 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}