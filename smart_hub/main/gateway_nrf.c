#include "gateway_nrf.h"
#include "device_control.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "gateway_nrf";

#define NRF_UART_PORT_NUM      UART_NUM_2
#define NRF_UART_BAUD_RATE     115200
#define NRF_UART_TXD           (GPIO_NUM_17)
#define NRF_UART_RXD           (GPIO_NUM_16)
#define NRF_UART_BUF_SIZE      1024

// Buffer de RX pentru UART (static, pentru simplitate)
static uint8_t rx_buf[NRF_UART_BUF_SIZE];

void gateway_nrf_init(void) {
    uart_config_t uart_config = {
        .baud_rate = NRF_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    ESP_ERROR_CHECK(uart_param_config(NRF_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(NRF_UART_PORT_NUM, NRF_UART_TXD, NRF_UART_RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(NRF_UART_PORT_NUM, NRF_UART_BUF_SIZE, 0, 0, NULL, 0));
    ESP_LOGI(TAG, "UART to nRF52840 initialized.");

    // Creează un task pentru RX de la nRF52840 (Zigbee/BLE)
    xTaskCreate(gateway_nrf_rx_task, "nrf_rx_task", 4096, NULL, 4, NULL);
}

// Trimite comenzi către nRF52840 și (opțional) citește răspunsul
int gateway_nrf_send_cmd(const char* cmd, char* resp_buf, int resp_bufsize) {
    uart_write_bytes(NRF_UART_PORT_NUM, cmd, strlen(cmd));
    uart_write_bytes(NRF_UART_PORT_NUM, "\n", 1);

    // Exemplu simplu: citire blocantă a răspunsului (timeout 500ms)
    int len = uart_read_bytes(NRF_UART_PORT_NUM, (uint8_t*)resp_buf, resp_bufsize-1, 500/portTICK_PERIOD_MS);
    if (len > 0) {
        resp_buf[len] = 0;
        return len;
    }
    if (resp_bufsize) resp_buf[0] = 0;
    return 0;
}

// Callback pentru device nou găsit prin nRF52840
void gateway_nrf_device_add(const char* id, const char* name, const char* type, const char* status) {
    device_t dev = {0};
    snprintf(dev.id, sizeof(dev.id), "nrf_%s", id);
    strncpy(dev.name, name, sizeof(dev.name)-1);
    strncpy(dev.type, type, sizeof(dev.type)-1);
    strcpy(dev.proto, "nrf");
    strncpy(dev.status, status, sizeof(dev.status)-1);
    snprintf(dev.extra, sizeof(dev.extra), "via nRF52840");
    device_add(&dev);
}

// Task care ascultă RX de la nRF52840 și parsează linii
static void gateway_nrf_rx_task(void* arg) {
    char line[256];
    int line_pos = 0;
    while (1) {
        uint8_t byte;
        int n = uart_read_bytes(NRF_UART_PORT_NUM, &byte, 1, 100 / portTICK_PERIOD_MS);
        if (n > 0) {
            if (byte == '\n' || line_pos >= sizeof(line) - 1) {
                line[line_pos] = 0;
                // Exemplu: nrf52840 trimite linii cu format "id;name;type;status"
                char id[64], name[64], type[32], status[16];
                if (sscanf(line, "%63[^;];%63[^;];%31[^;];%15s", id, name, type, status) == 4) {
                    ESP_LOGI(TAG, "nRF Device: %s %s %s %s", id, name, type, status);
                    gateway_nrf_device_add(id, name, type, status);
                }
                line_pos = 0;
            } else {
                line[line_pos++] = byte;
            }
        }
    }
}
