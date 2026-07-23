/**
 * @file network_manager.c
 * @brief Implementation of network initialization, dynamic AP lifecycle control, and STA testing.
 */

#include "network_manager.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "NET_MGR";

static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
#define MAXIMUM_RETRY 5

/**
 * @brief Internal event handler for Wi-Fi and IP events during STA connection testing.
 * 
 * @param arg User-defined context pointer.
 * @param event_base Event category base identifier.
 * @param event_id Specific event subtype identifier.
 * @param event_data Event payload data.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGW(TAG, "Retry connecting to AP (%d/%d)", s_retry_num, MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Initializes network interfaces, system event loop, and the Wi-Fi driver.
 */
esp_err_t network_manager_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    return esp_wifi_init(&cfg);
}

/**
 * @brief Configures and starts the ESP32 in Access Point (AP) mode.
 */
esp_err_t network_manager_start_ap(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    
    wifi_config_t wifi_config = {
        .ap = {
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    
    snprintf((char*)wifi_config.ap.ssid, sizeof(wifi_config.ap.ssid), "BMS_CONFIG_%02X%02X", mac[4], mac[5]);
    wifi_config.ap.ssid_len = strlen((char*)wifi_config.ap.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP Mode started successfully. SSID: %s", wifi_config.ap.ssid);
    return ESP_OK;
}

/**
 * @brief Stops active AP mode operation without resetting the entire device.
 */
esp_err_t network_manager_stop_ap(void)
{
    ESP_LOGI(TAG, "Stopping AP mode...");
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_LOGI(TAG, "AP mode stopped successfully.");
    return ESP_OK;
}

/**
 * @brief Synchronously tests connection to a specified Wi-Fi router.
 */
bool network_manager_test_sta_connection(const char* ssid, const char* password)
{
    wifi_event_group = xEventGroupCreate();

    esp_wifi_stop();
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = {0};
    strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    s_retry_num = 0;
    ESP_LOGI(TAG, "Testing connection to SSID: %s...", ssid);
    esp_wifi_start();

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(10000));

    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    vEventGroupDelete(wifi_event_group);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Wi-Fi test passed successfully.");
        return true;
    } else {
        ESP_LOGE(TAG, "Wi-Fi test failed or timed out.");
        return false;
    }
}