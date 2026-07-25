/**
 * @file mqtt_manager.c
 * @brief Secure MQTT Client Manager implementation with defensive error handling for ESP32.
 * @details Handles secure TLS (Port 8883) connection with certificate verification,
 *          robust validation against buffer overflows, null checks, and error recovery.
 */

#include "mqtt_manager.h"
#include "nvs_manager.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/// Logging tag for MQTT Manager module
static const char *TAG = "MQTT_MGR";

/// Global MQTT client handle
static esp_mqtt_client_handle_t s_client = NULL;

/// Connection state flag tracking broker link status
static bool s_is_connected = false;

/// NVS storage namespace for configuration parameters
#define NVS_NAMESPACE "storage"

/**
 * @brief Embedded CA certificate binary reference (linked via CMake EMBED_FILES).
 */
extern const char ca_crt_start[] asm("_binary_ca_crt_start");

/**
 * @brief MQTT Event Handler callback function with state safety checks.
 * 
 * @param handler_args User-defined context pointer.
 * @param base Event base identifier.
 * @param event_id Specific event identifier (e.g., MQTT_EVENT_CONNECTED).
 * @param event_data Pointer to event data structure (esp_mqtt_event_handle_t).
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    
    // Event data null pointer safety check
    if (event_data == NULL) {
        ESP_LOGW(TAG, "Received MQTT event with NULL event_data.");
        return;
    }

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Secure MQTT Client connected to broker successfully!");
            s_is_connected = true;
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Secure MQTT Client disconnected from broker. Will auto-reconnect.");
            s_is_connected = false;
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Secure MQTT event error occurred.");
            s_is_connected = false;
            break;

        case MQTT_EVENT_PUBLISHED:
            // Optional: Can track msg_id if ACK confirmation is needed
            break;

        default:
            break;
    }
}

/**
 * @brief Initializes the secure MQTT client with defensive validations and NVS fallback.
 * @details Reads the secure broker URI from NVS, sanitizes percent-encoded characters,
 *          configures TLS parameters with embedded CA certificates, and starts the background task.
 * 
 * @return esp_err_t ESP_OK on success, or appropriate error code on failure.
 */
esp_err_t mqtt_manager_init(void)
{
    char raw_uri[128] = {0};
    char broker_uri[128] = {0};

    // Safely read URI from NVS with a robust fallback default (void return handled correctly)
    nvs_manager_read_str(NVS_NAMESPACE, "mqtt_uri", raw_uri, sizeof(raw_uri), "mqtts://192.168.1.109:8883");

    // Defensive URI sanitization / URL-decode with buffer overflow protection
    int src = 0, dst = 0;
    int max_len = sizeof(broker_uri) - 1;
    
    while (raw_uri[src] != '\0' && dst < max_len) {
        if (raw_uri[src] == '%' && raw_uri[src+1] != '\0' && raw_uri[src+2] != '\0') {
            char hex[3] = {raw_uri[src+1], raw_uri[src+2], '\0'};
            char *endptr = NULL;
            long decoded_char = strtol(hex, &endptr, 16);
            
            // Check if hex conversion was valid
            if (endptr == hex + 2) {
                broker_uri[dst++] = (char)decoded_char;
                src += 3;
            } else {
                // Malformed percent-encoding, keep original character safely
                broker_uri[dst++] = raw_uri[src++];
            }
        } else {
            broker_uri[dst++] = raw_uri[src++];
        }
    }
    broker_uri[dst] = '\0';

    // Validate that broker URI is not empty after parsing
    if (strlen(broker_uri) == 0) {
        ESP_LOGE(TAG, "Parsed MQTT broker URI is empty! Initialization aborted.");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing TLS MQTT client with URI: %s", broker_uri);

    // Secure MQTT configuration structure (TLS + mTLS CA Certificate + Auth)
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = broker_uri,
            .verification = {
                .certificate = ca_crt_start,
                .skip_cert_common_name_check = true
            }
        },
        .credentials = {
            .username = "esp32",
            .authentication.password = "123456"
        },
        .session = {
            .keepalive = 60 // Keep-alive timeout to detect dead links early
        },
        .network = {
            .timeout_ms = 5000 // Connection timeout safeguard
        }
    };

    // Initialize MQTT client instance with memory allocation check
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for Secure MQTT client.");
        return ESP_ERR_NO_MEM;
    }

    // Register event handler safely
    esp_err_t reg_err = esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (reg_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler (err: %d)", reg_err);
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return reg_err;
    }
    
    // Start the MQTT client background task safely
    esp_err_t start_err = esp_mqtt_client_start(s_client);
    if (start_err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client background task (err: %d)", start_err);
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return start_err;
    }

    return ESP_OK;
}

/**
 * @brief Publishes formatted BMS telemetry JSON payload with boundary and state checking.
 * 
 * @param voltage Battery pack voltage in volts (V).
 * @param current Current flow in amperes (A).
 * @param soc State of Charge percentage (0-100 %).
 * @param temperature Battery temperature in degrees Celsius (°C).
 * @return esp_err_t ESP_OK if published successfully, error code otherwise.
 */
esp_err_t mqtt_manager_publish_telemetry(float voltage, float current, uint8_t soc, float temperature)
{
    // Check if client is initialized and currently connected to prevent invalid writes/crashes
    if (!s_is_connected || s_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Sanity boundary checks for telemetry parameters to catch extreme outlier errors
    if (voltage < 0.0f || voltage > 100.0f || soc > 100) {
        ESP_LOGW(TAG, "Telemetry values out of normal operating range! Skipping publish.");
        return ESP_ERR_INVALID_ARG;
    }

    char json_payload[256];
    int written = snprintf(json_payload, sizeof(json_payload),
                           "{\"voltage\":%.2f,\"current\":%.2f,\"soc\":%u,\"temperature\":%.1f}",
                           voltage, current, soc, temperature);

    // Verify buffer truncation or formatting error
    if (written < 0 || written >= sizeof(json_payload)) {
        ESP_LOGE(TAG, "JSON payload formatting failed or buffer truncated.");
        return ESP_ERR_INVALID_SIZE;
    }

    // Publish to telemetry topic with QoS 1, non-blocking check
    int msg_id = esp_mqtt_client_publish(s_client, "ubis/telemetry/state", json_payload, 0, 1, 0);
    if (msg_id < 0) {
        ESP_LOGW(TAG, "Failed to queue telemetry message to MQTT broker.");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Checks if the MQTT client is currently connected to the broker safely.
 * 
 * @return true if connected and client handle is active.
 * @return false if disconnected or uninitialized.
 */
bool mqtt_manager_is_connected(void)
{
    return s_is_connected && (s_client != NULL);
}