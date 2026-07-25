/**
 * @file bms_manager.c
 * @brief Implementation of BMS Manager orchestrator and driver selection pattern.
 */

#include "bms_manager.h"
#include "bms_driver_jbd.h"
#include "nvs_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "BMS_MGR";
static const bms_driver_t *s_active_driver = NULL;

#define NVS_NAMESPACE "storage"

esp_err_t bms_manager_init(void)
{
    char bms_type[16] = {0};

    // Read configured BMS type from NVS (default to "JBD" if not set or AUTO)
    nvs_manager_read_str(NVS_NAMESPACE, "bms_type", bms_type, sizeof(bms_type), "JBD");

    ESP_LOGI(TAG, "Configured BMS Type from NVS: %s", bms_type);

    if (strcmp(bms_type, "JBD") == 0 || strcmp(bms_type, "AUTO") == 0) {
        s_active_driver = bms_driver_jbd_get_driver();
    } else {
        ESP_LOGW(TAG, "Unknown or unsupported BMS type '%s'. Defaulting to JBD.", bms_type);
        s_active_driver = bms_driver_jbd_get_driver();
    }

    if (s_active_driver && s_active_driver->init) {
        ESP_LOGI(TAG, "Initializing active BMS driver: %s", s_active_driver->name);
        return s_active_driver->init();
    }

    ESP_LOGE(TAG, "No active BMS driver could be initialized!");
    return ESP_FAIL;
}

esp_err_t bms_manager_read(bms_data_t *data)
{
    if (s_active_driver && s_active_driver->read_data) {
        return s_active_driver->read_data(data);
    }
    return ESP_FAIL;
}