/**
 * @file button_manager.c
 * @brief Implementation of robust 5+ second long-press BOOT button monitor for AP provisioning mode.
 */

#include "button_manager.h"
#include "nvs_manager.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

/// Logging tag for Button Manager module
static const char *TAG = "BTN_MGR";

#define CONFIG_RESET_BUTTON_GPIO    0     ///< BOOT button on ESP32 DevKit
#define BUTTON_ACTIVE_LEVEL         0     ///< Active Low (0 when pressed)
#define NVS_NAMESPACE               "storage"

/// FreeRTOS software timer handle for measuring the 5-second threshold
static TimerHandle_t s_button_timer = NULL;

/**
 * @brief Timer callback triggered at the 5-second mark of continuous press.
 * 
 * @param xTimer Timer handle (unused).
 */
static void button_long_press_timer_callback(TimerHandle_t xTimer)
{
    (void)xTimer;

    // Double check state with stability reading to prevent floating/noise false positives
    int current_level = gpio_get_level(CONFIG_RESET_BUTTON_GPIO);
    
    if (current_level == BUTTON_ACTIVE_LEVEL) {
        ESP_LOGW(TAG, "BOOT button stably held for 5+ seconds! Triggering Factory Reset / AP Mode...");

        // Clear Wi-Fi credentials via nvs_manager
        nvs_manager_write_str(NVS_NAMESPACE, "wifi_ssid", "");
        nvs_manager_write_str(NVS_NAMESPACE, "wifi_pass", "");
        nvs_manager_write_str(NVS_NAMESPACE, "bms_type", "");
        nvs_manager_write_str(NVS_NAMESPACE, "mqtt_uri", "");
        ESP_LOGI(TAG, "Stored Wi-Fi credentials cleared.");

        // Reboot into AP provisioning mode
        ESP_LOGI(TAG, "Rebooting device into Provisioning AP mode...");
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
    } else {
        ESP_LOGI(TAG, "Button released before 5 seconds threshold.");
    }
}

/**
 * @brief GPIO Interrupt Service Routine (ISR) managing press and release events.
 * 
 * @param arg User parameter (unused).
 */
static void IRAM_ATTR button_gpio_isr_handler(void *arg)
{
    (void)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int level = gpio_get_level(CONFIG_RESET_BUTTON_GPIO);

    if (s_button_timer != NULL) {
        if (level == BUTTON_ACTIVE_LEVEL) {
            // Button pressed (Falling edge): Start the 5-second minimum threshold timer
            xTimerResetFromISR(s_button_timer, &xHigherPriorityTaskWoken);
        } else {
            // Button released (Rising edge): Stop the timer immediately
            xTimerStopFromISR(s_button_timer, &xHigherPriorityTaskWoken);
        }
    }

    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/**
 * @brief Initializes the hardware BOOT button GPIO and configures the long-press interrupt monitor.
 * 
 * @return esp_err_t ESP_OK on success, or ESP_ERR_NO_MEM if timer allocation fails.
 */
esp_err_t button_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing BOOT button manager for provisioning reset...");

    // Create a 5-second software timer (non-auto-reload)
    s_button_timer = xTimerCreate(
        "btn_timer",
        pdMS_TO_TICKS(5000), // Minimum 5 seconds threshold
        pdFALSE,
        (void *)0,
        button_long_press_timer_callback
    );

    if (s_button_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create button timer!");
        return ESP_ERR_NO_MEM;
    }

    // Configure GPIO 0 with internal pull-up and any-edge interrupts
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_RESET_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Pull-up active to prevent floating
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE       // Trigger on both press and release
    };
    gpio_config(&io_conf);

    // Install GPIO ISR service and add handler
    gpio_install_isr_service(0);
    gpio_isr_handler_add(CONFIG_RESET_BUTTON_GPIO, button_gpio_isr_handler, NULL);

    ESP_LOGI(TAG, "BOOT button manager successfully initialized with interrupts.");
    return ESP_OK;
}