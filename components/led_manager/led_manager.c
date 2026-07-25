/**
 * @file led_manager.c
 * @brief Implementation of non-blocking FreeRTOS-backed status LED manager with thread safety.
 */

#include "led_manager.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/// Logging tag for LED Manager module
static const char *TAG = "LED_MGR";

#define CONFIG_STATUS_LED_GPIO   2     ///< Onboard user LED (GPIO 2 on many ESP32 DevKits)

/// Current active operational mode of the status LED
static led_mode_t s_current_mode = LED_MODE_OFF;
/// Handle for the background LED pattern execution task
static TaskHandle_t s_led_task_handle = NULL;
/// Mutex handle for thread-safe mode switching
static SemaphoreHandle_t s_led_mutex = NULL;

/**
 * @brief Background task running non-blocking LED patterns based on current mode.
 * 
 * @param pvParameters Task runtime parameters (unused).
 */
static void led_task(void *pvParameters)
{
    (void)pvParameters;
    bool led_state = false;
    led_mode_t local_mode;

    while (1) {
        // Safely fetch current mode using mutex to prevent race conditions
        if (xSemaphoreTake(s_led_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            local_mode = s_current_mode;
            xSemaphoreGive(s_led_mutex);
        } else {
            local_mode = s_current_mode; // Fallback
        }

        switch (local_mode) {
            case LED_MODE_OFF:
                gpio_set_level(CONFIG_STATUS_LED_GPIO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_MODE_ON:
                gpio_set_level(CONFIG_STATUS_LED_GPIO, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_MODE_PROVISIONING:
                // Fast blink (200ms ON, 200ms OFF)
                led_state = !led_state;
                gpio_set_level(CONFIG_STATUS_LED_GPIO, led_state ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(200));
                break;

            case LED_MODE_ERROR:
                // Slow blink (1000ms ON, 1000ms OFF)
                led_state = !led_state;
                gpio_set_level(CONFIG_STATUS_LED_GPIO, led_state ? 1 : 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;

            default:
                gpio_set_level(CONFIG_STATUS_LED_GPIO, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
        }
    }
}

/**
 * @brief Initializes the onboard LED GPIO and starts the background control task.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t led_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing status LED manager on GPIO %d...", CONFIG_STATUS_LED_GPIO);

    // Create mutex for thread-safe state protection
    s_led_mutex = xSemaphoreCreateMutex();
    if (s_led_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LED mutex!");
        return ESP_ERR_NO_MEM;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << CONFIG_STATUS_LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(CONFIG_STATUS_LED_GPIO, 0);

    // Spawn non-blocking background task for LED patterns
    BaseType_t ret = xTaskCreate(
        led_task,
        "led_task",
        2048,
        NULL,
        1, // Low priority background task
        &s_led_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED task!");
        vSemaphoreDelete(s_led_mutex);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "LED manager initialized successfully.");
    return ESP_OK;
}

/**
 * @brief Sets the current operational mode of the status LED thread-safely.
 * 
 * @param mode Target LED mode (`led_mode_t`).
 */
void led_manager_set_mode(led_mode_t mode)
{
    if (s_led_mutex == NULL) {
        s_current_mode = mode;
        return;
    }

    if (xSemaphoreTake(s_led_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (s_current_mode != mode) {
            s_current_mode = mode;
            ESP_LOGI(TAG, "LED mode changed to: %d", mode);
        }
        xSemaphoreGive(s_led_mutex);
    }
}