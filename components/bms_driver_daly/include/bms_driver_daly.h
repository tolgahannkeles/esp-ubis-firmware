/**
 * @file bms_driver_daly.h
 * @brief Daly BMS driver header for polymorphic integration.
 */

#pragma once

#include "bms_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns the singleton pointer to the Daly BMS driver structure.
 * 
 * @return const bms_driver_t* Pointer to the Daly driver interface.
 */
const bms_driver_t* bms_driver_daly_get_driver(void);

#ifdef __cplusplus
}
#endif