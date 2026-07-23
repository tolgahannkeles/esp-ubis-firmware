#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

/**
 * @file nvs_manager.h
 * @brief Non-Volatile Storage (NVS) wrapper for ESP UBIS project.
 *        Provides simplified, safe read/write operations for configuration data.
 */

/**
 * @brief Initializes the default NVS partition.
 *        If the partition is corrupted or contains a new version, it will be erased and re-initialized automatically.
 * 
 * @return 
 *      - ESP_OK on success
 *      - esp_err_t error code on failure
 */
esp_err_t nvs_manager_init(void);

/**
 * @brief Writes a 32-bit integer to NVS.
 * 
 * @param namespace_name NVS namespace name (maximum 15 characters).
 * @param key Key name (maximum 15 characters).
 * @param value The 32-bit integer value to store.
 * @return 
 *      - ESP_OK on success
 *      - esp_err_t error code on failure
 */
esp_err_t nvs_manager_write_i32(const char* namespace_name, const char* key, int32_t value);

/**
 * @brief Reads a 32-bit integer from NVS.
 * 
 * @param namespace_name NVS namespace name.
 * @param key Key name to look for.
 * @param default_value Value to return if the key is not found or an error occurs.
 * @return 
 *      - The stored integer value
 *      - default_value (if not found)
 */
int32_t nvs_manager_read_i32(const char* namespace_name, const char* key, int32_t default_value);

/**
 * @brief Writes a null-terminated string to NVS.
 * 
 * @param namespace_name NVS namespace name.
 * @param key Key name.
 * @param value Null-terminated string to store.
 * @return 
 *      - ESP_OK on success
 *      - esp_err_t error code on failure
 */
esp_err_t nvs_manager_write_str(const char* namespace_name, const char* key, const char* value);

/**
 * @brief Reads a string from NVS into a provided buffer.
 *        If the key is not found, the default_value is copied into the buffer.
 * 
 * @param namespace_name NVS namespace name.
 * @param key Key name to look for.
 * @param out_value Pointer to the buffer where the string will be copied.
 * @param max_len Maximum allowed length to prevent buffer overflow (including null-terminator).
 * @param default_value String to copy if the key does not exist.
 */
void nvs_manager_read_str(const char* namespace_name, const char* key, char* out_value, size_t max_len, const char* default_value);