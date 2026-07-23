/**
 * @file nvs_manager.c
 * @brief Implementation of the NVS manager for the ESP UBIS project.
 *        Handles low-level NVS operations, memory allocation, and error recovery.
 */

#include "nvs_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "NVS_MGR";

esp_err_t nvs_manager_init(void) {
    // Attempt to initialize the default NVS partition
    esp_err_t ret = nvs_flash_init();
    
    // Check if NVS partition was truncated or contains a newer version of the data format.
    // This usually happens after an OTA update or if the partition table is changed.
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition is invalid or full. Erasing and re-initializing...");
        
        // Erase the partition completely to prevent boot loops
        ESP_ERROR_CHECK(nvs_flash_erase());
        
        // Retry initialization after formatting
        ret = nvs_flash_init();
    }
    
    return ret;
}

esp_err_t nvs_manager_write_i32(const char* namespace_name, const char* key, int32_t value) {
    nvs_handle_t handle;
    
    // Open NVS in read/write mode for the specific namespace
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    // Set the value in RAM
    err = nvs_set_i32(handle, key, value);
    
    // If successfully set, commit the changes to the physical flash memory
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    // Always close the handle to prevent memory leaks
    nvs_close(handle);
    return err;
}

int32_t nvs_manager_read_i32(const char* namespace_name, const char* key, int32_t default_value) {
    nvs_handle_t handle;
    int32_t value = default_value;
    
    // Open NVS in read-only mode to save resources
    if (nvs_open(namespace_name, NVS_READONLY, &handle) == ESP_OK) {
        // Attempt to read the key. If it doesn't exist, 'value' remains untouched.
        esp_err_t err = nvs_get_i32(handle, key, &value);
        if (err != ESP_OK) {
            value = default_value; // Enforce default value on read failure
        }
        nvs_close(handle);
    }
    
    return value;
}

esp_err_t nvs_manager_write_str(const char* namespace_name, const char* key, const char* value) {
    nvs_handle_t handle;
    
    // Open NVS in read/write mode
    esp_err_t err = nvs_open(namespace_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    // Write the null-terminated string to RAM
    err = nvs_set_str(handle, key, value);
    
    // Commit to flash memory
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    
    nvs_close(handle);
    return err;
}

void nvs_manager_read_str(const char* namespace_name, const char* key, char* out_value, size_t max_len, const char* default_value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace_name, NVS_READONLY, &handle);
    
    if (err == ESP_OK) {
        // Pass the maximum buffer size to prevent buffer overflows.
        // NVS will update required_size with the actual string length read.
        size_t required_size = max_len;
        err = nvs_get_str(handle, key, out_value, &required_size);
        nvs_close(handle);
    }
    
    // Fallback mechanism: If opening failed, key was not found, or buffer was too small
    if (err != ESP_OK) {
        // Safely copy the default string into the provided buffer
        strncpy(out_value, default_value, max_len);
        
        // Ensure the string is strictly null-terminated even if default_value exceeds max_len
        out_value[max_len - 1] = '\0';
    }
}