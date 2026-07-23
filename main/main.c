/**
 * @file main.c
 * @brief Main application entry point for comprehensive testing of network_manager functions.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_manager.h"
#include "network_manager.h"

static const char *TAG = "UBIS_MAIN";

/**
 * @brief Application main execution sequence testing init, AP start, AP stop, and STA connection.
 */
void app_main(void)
{
    // NVS must be initialized first as required by the underlying ESP-IDF Wi-Fi driver
    ESP_ERROR_CHECK(nvs_manager_init());
    ESP_LOGI(TAG, "NVS initialized.");

    // Step 1: Test network manager initialization
    ESP_ERROR_CHECK(network_manager_init());
    ESP_LOGI(TAG, "TEST PASSED: Network manager initialized successfully.");

    // Step 2: Test starting AP mode
    ESP_LOGI(TAG, "--- STARTING AP MODE TEST ---");
    ESP_ERROR_CHECK(network_manager_start_ap());
    ESP_LOGI(TAG, "TEST PASSED: AP mode is running. Check your phone for 'BMS_CONFIG_XXXX'.");

    // Keep AP mode active for 10 seconds for visual verification
    vTaskDelay(pdMS_TO_TICKS(10000));

    // Step 3: Test stopping AP mode dynamically without rebooting
    ESP_LOGI(TAG, "--- STOPPING AP MODE TEST ---");
    ESP_ERROR_CHECK(network_manager_stop_ap());
    ESP_LOGI(TAG, "TEST PASSED: AP mode stopped cleanly without resetting the device.");

    vTaskDelay(pdMS_TO_TICKS(2000));

    // Step 4: Test Station (STA) connection validation
    ESP_LOGI(TAG, "--- STARTING STA CONNECTION TEST ---");
    const char *test_ssid = "WIFI_SSID";
    const char *test_pass = "PASSWORD";

    ESP_LOGI(TAG, "Attempting connection to router SSID: %s", test_ssid);
    bool sta_result = network_manager_test_sta_connection(test_ssid, test_pass);

    if (sta_result) {
        ESP_LOGI(TAG, "TEST PASSED: Successfully connected to router and obtained IP address!");
    } else {
        ESP_LOGE(TAG, "TEST FAILED: Could not connect to router (check SSID/password or signal range).");
    }

    // Main idle loop to keep task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}