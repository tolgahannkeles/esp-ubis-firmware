/**
 * @file bms_driver_jbd.h
 * @brief JBD (Xiaoxiang) BMS UART driver header.
 */

#pragma once

#include "bms_interface.h"

/**
 * @brief Retrieves the JBD BMS driver interface implementation.
 * 
 * @return const bms_driver_t* Pointer to the polymorphic driver structure.
 */
const bms_driver_t* bms_driver_jbd_get_driver(void);