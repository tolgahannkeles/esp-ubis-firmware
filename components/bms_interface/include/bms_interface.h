/**
 * @file bms_interface.h
 * @brief Abstract BMS interface definition for polymorphic driver management.
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Standardized BMS telemetry data structure.
 */
typedef struct {
    float voltage;       // Total pack voltage (V)
    float current;       // Pack current (A) (+: Charging, -: Discharging)
    float temperature;   // Temperature (°C)
    uint8_t soc;         // State of Charge (%)
    bool is_online;      // Communication status flag
} bms_data_t;

/**
 * @brief BMS driver interface structure (Strategy Pattern / Virtual Table).
 */
typedef struct {
    const char* name;                              // Driver name (e.g., "JBD", "DALY")
    esp_err_t (*init)(void);                       // Initialization function (UART/CAN setup)
    esp_err_t (*read_data)(bms_data_t *data);      // Fetch telemetry data function
} bms_driver_t;