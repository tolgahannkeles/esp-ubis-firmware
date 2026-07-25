/**
 * @file led_manager.h
 * @brief LED manager header for status indication (Provisioning, Errors, Normal operation).
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LED operational status modes.
 */
typedef enum {
    LED_MODE_OFF = 0,               ///< LED completely off
    LED_MODE_ON,                    ///< LED solid on
    LED_MODE_PROVISIONING,          ///< Fast blinking (AP mode / waiting for Wi-Fi)
    LED_MODE_ERROR,                 ///< Slow blinking (Connection/system error)
} led_mode_t;

/**
 * @brief Initializes the onboard LED GPIO and starts the background control task.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t led_manager_init(void);

/**
 * @brief Sets the current operational mode of the status LED.
 * 
 * @param mode Target LED mode (`led_mode_t`).
 */
void led_manager_set_mode(led_mode_t mode);

#ifdef __cplusplus
}
#endif