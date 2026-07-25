/**
 * @file bms_driver_daly.c
 * @brief Daly BMS UART protocol implementation.
 */

#include "bms_driver_daly.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/// Logging tag for Daly BMS driver module
static const char *TAG = "BMS_DRIVER_DALY";

#define UART_PORT_NUM      UART_NUM_1
#define UART_TX_PIN        17  ///< ESP32 TX pin
#define UART_RX_PIN        16  ///< ESP32 RX pin
#define UART_BAUD_RATE     9600
#define BUF_SIZE           1024

/// Daly Basic Info Request Command Frame
static const uint8_t DALY_CMD_BASIC_INFO[] = {0xA5, 0x40, 0x90, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D};

/**
 * @brief Initializes UART peripheral for Daly BMS communication.
 * 
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
static esp_err_t daly_init(void)
{
    ESP_LOGI(TAG, "Initializing UART for Daly BMS on port %d...", UART_PORT_NUM);

    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_param_config(UART_PORT_NUM, &uart_config);
    if (err != ESP_OK) return err;

    err = uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;

    return uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
}

/**
 * @brief Sends telemetry request command and parses incoming Daly response frame.
 * 
 * @param data Pointer to store parsed telemetry data.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
static esp_err_t daly_read_data(bms_data_t *data)
{
    if (!data) return ESP_ERR_INVALID_ARG;

    uart_flush_input(UART_PORT_NUM);

    int sent = uart_write_bytes(UART_PORT_NUM, (const char*)DALY_CMD_BASIC_INFO, sizeof(DALY_CMD_BASIC_INFO));
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send command to Daly BMS");
        data->is_online = false;
        return ESP_FAIL;
    }

    uint8_t rx_buf[64];
    int length = uart_read_bytes(UART_PORT_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(200));

    if (length < 13) {
        ESP_LOGW(TAG, "Daly response timeout or invalid length (%d bytes)", length);
        data->is_online = false;
        return ESP_ERR_TIMEOUT;
    }

    if (rx_buf[0] == 0xA5 && rx_buf[1] == 0x90) {
        uint16_t raw_voltage = (rx_buf[4] << 8) | rx_buf[5];
        data->voltage = (float)raw_voltage * 0.1f;

        uint16_t raw_current = (rx_buf[8] << 8) | rx_buf[9];
        data->current = ((float)raw_current - 30000.0f) * 0.1f;

        uint16_t raw_soc = (rx_buf[10] << 8) | rx_buf[11];
        data->soc = (uint8_t)(raw_soc / 10);

        data->temperature = (float)((int)rx_buf[12] - 40);

        data->is_online = true;
        return ESP_OK;
    }

    data->is_online = false;
    return ESP_FAIL;
}

/// Daly driver instance mapping structure
static const bms_driver_t daly_driver = {
    .name = "DALY",
    .init = daly_init,
    .read_data = daly_read_data
};

/**
 * @brief Returns the singleton pointer to the Daly BMS driver structure.
 * 
 * @return const bms_driver_t* Pointer to the Daly driver interface.
 */
const bms_driver_t* bms_driver_daly_get_driver(void)
{
    return &daly_driver;
}