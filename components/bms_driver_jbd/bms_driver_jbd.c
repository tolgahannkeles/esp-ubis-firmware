/**
 * @file bms_driver_jbd.c
 * @brief JBD (Xiaoxiang) BMS UART protocol implementation.
 */

#include "bms_driver_jbd.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "BMS_DRIVER_JBD";

#define UART_PORT_NUM      UART_NUM_1
#define UART_TX_PIN        17  // ESP32 TX pin
#define UART_RX_PIN        16  // ESP32 RX pin
#define UART_BAUD_RATE     9600
#define BUF_SIZE           1024

// JBD Basic Information Request Command (Read Registers / Basic Info)
static const uint8_t JBD_CMD_BASIC_INFO[] = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};

/**
 * @brief Initializes UART peripheral for JBD BMS communication.
 */
static esp_err_t jbd_init(void)
{
    ESP_LOGI(TAG, "Initializing UART for JBD BMS on port %d...", UART_PORT_NUM);

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
 * @brief Sends telemetry request command and parses incoming JBD response frame.
 */
static esp_err_t jbd_read_data(bms_data_t *data)
{
    if (!data) return ESP_ERR_INVALID_ARG;

    // Clear buffer and flush
    uart_flush_input(UART_PORT_NUM);

    // Send request command to JBD BMS
    int sent = uart_write_bytes(UART_PORT_NUM, (const char*)JBD_CMD_BASIC_INFO, sizeof(JBD_CMD_BASIC_INFO));
    if (sent < 0) {
        ESP_LOGE(TAG, "Failed to send command to JBD BMS");
        data->is_online = false;
        return ESP_FAIL;
    }

    uint8_t rx_buf[64];
    // Read response with timeout
    int length = uart_read_bytes(UART_PORT_NUM, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(200));

    if (length < 30) {
        ESP_LOGW(TAG, "JBD response timeout or invalid length (%d bytes)", length);
        data->is_online = false;
        return ESP_ERR_TIMEOUT;
    }

    // JBD Frame Check: Start(0xDD), Status(0x00)
    if (rx_buf[0] == 0xDD && rx_buf[1] == 0x00) {
        // rx_buf[3], [4]: Voltage (mV -> V, scale 0.01)
        uint16_t raw_voltage = (rx_buf[3] << 8) | rx_buf[4];
        data->voltage = (float)raw_voltage * 0.01f;

        // rx_buf[5], [6]: Current (Signed conversion)
        uint16_t raw_current = (uint16_t)((rx_buf[5] << 8) | rx_buf[6]);
        if (raw_current & 0x8000) {
            data->current = (float)((int16_t)raw_current) * 0.01f;
        } else {
            data->current = (float)raw_current * 0.01f;
        }

        // rx_buf[25]: SoC percentage
        data->soc = rx_buf[25];

        // rx_buf[28], [29]: Temperature (Kelvin to Celsius conversion)
        uint16_t raw_temp = (rx_buf[28] << 8) | rx_buf[29];
        data->temperature = ((float)raw_temp * 0.1f) - 273.15f;

        data->is_online = true;
        return ESP_OK;
    }

    data->is_online = false;
    return ESP_FAIL;
}

// Singleton driver instance mapping
static const bms_driver_t jbd_driver = {
    .name = "JBD",
    .init = jbd_init,
    .read_data = jbd_read_data
};

const bms_driver_t* bms_driver_jbd_get_driver(void)
{
    return &jbd_driver;
}