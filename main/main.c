/**
 * @file main.c
 * @brief Main application entry point and system orchestration for ESP UBIS firmware.
 * @details Sequentially initializes non-volatile storage, network interfaces, 
 *          provisioning portal, secure MQTT client, BMS driver layer, application worker tasks,
 *          hardware button manager, and status LED manager.
 */

#include <stdio.h>
#include "esp_log.h"
#include "nvs_manager.h"
#include "network_manager.h"
#include "web_server.h"
#include "bms_manager.h"
#include "mqtt_manager.h"
#include "app_task.h"
#include "button_manager.h"
#include "led_manager.h"

/// Logging tag for Main Application module
static const char *TAG = "UBIS_MAIN";

/**
 * @brief Application entry point. Initializes all system subsystems sequentially and safeguards startup.
 */
void app_main(void)
{
    ESP_LOGI(TAG, "Starting UBIS BMS Firmware boot sequence...");

    // 1. Initialize Status LED Manager FIRST (so we can indicate status immediately)
    esp_err_t err = led_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LED manager! (err: %d)", err);
    } else {
        ESP_LOGI(TAG, "LED manager initialized successfully.");
        // Initially set to provisioning mode as default.
        led_manager_set_mode(LED_MODE_PROVISIONING);
    }

    // 2. Initialize Non-Volatile Storage (NVS) subsystem
    err = nvs_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS subsystem! (err: %d)", err);
        led_manager_set_mode(LED_MODE_ERROR);
    } else {
        ESP_LOGI(TAG, "NVS subsystem successfully initialized.");
    }

    // 3. Initialize Network Manager (Wi-Fi stack and network interfaces)
    // Note: network_manager connects if credentials exist, otherwise falls back to AP provisioning mode.
    err = network_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize Network Manager! (err: %d)", err);
        led_manager_set_mode(LED_MODE_ERROR);
    } else {
        ESP_LOGI(TAG, "Network manager driver initialized.");
    }

    // 4. Initialize Web Server / Provisioning Portal & Verify Stored Credentials
    err = web_server_init_portal();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Provisioning portal initialization reported warnings/errors (err: %d)", err);
    } else {
        ESP_LOGI(TAG, "Provisioning portal initialized / verified successfully.");
    }

    // 5. Initialize Secure MQTT Client Manager (TLS + Authentication)
    err = mqtt_manager_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "MQTT manager initialization failed or waiting for network link (err: %d)", err);
    } else {
        ESP_LOGI(TAG, "MQTT manager initialized successfully.");
    }

    // 6. Initialize BMS Hardware Driver Layer (Polymorphic driver selection via NVS)
    err = bms_manager_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "BMS manager initialized successfully.");
    } else {
        ESP_LOGE(TAG, "Failed to initialize BMS manager! (err: %d)", err);
        led_manager_set_mode(LED_MODE_ERROR);
    }

    // 7. Initialize Main Application Worker (Business Logic & Telemetry Pipeline Task)
    err = app_task_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CRITICAL: Failed to initialize application worker task! (err: %d)", err);
        led_manager_set_mode(LED_MODE_ERROR);
    } else {
        ESP_LOGI(TAG, "Application worker task spawned successfully.");
    }

    // 8. Initialize Hardware BOOT Button Manager (Interrupt-driven 5s long-press reset)
    err = button_manager_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize button manager! (err: %d)", err);
    } else {
        ESP_LOGI(TAG, "Button manager initialized successfully.");
    }

    ESP_LOGI(TAG, "Firmware full boot sequence completed successfully.");
}