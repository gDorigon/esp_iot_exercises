#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_2, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE,
                 UART_PIN_NO_CHANGE);
}

int sendData(const char *logName, const char *data)
{

    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_2, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    char c = 0;
    char *str = (char *)malloc(100);
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (c != '\n')
    {
        // Recebe dado pela serial
        c = getchar();

        // Caso a tecla seja "backspace"
        if (c == 0x08)
        {
            // Apaga o último caracter da string
            str[((strlen(str) - 1) > 0) ? strlen(str) - 1 : 0] = c;

            // Apaga o conteúdo da linha no terminal
            printf("\x1b[2K");

            // Imprime conteúdo de str no terminal
            printf("\rDigite: %.*s", strlen(str), str);
        }
        // Caso seja caracter válido
        else if ((c >= 0x20) && (c <= 0x7e))
        {
            // Insere caracter na ultima posição de str
            str[strlen(str)] = c;

            // Apaga o conteúdo da linha no terminal
            printf("\x1b[2K");

            // Imprime conteúdo de str no terminal
            printf("\rDigite: %.*s", strlen(str), str);
        }
    }
    sendData(TX_TASK_TAG, str);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    if (str)
        free(str);
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    while (1)
    {
        const int rxBytes = uart_read_bytes(UART_NUM_2, data, RX_BUF_SIZE,
                                            1000 / portTICK_RATE_MS);
        if (rxBytes > 0)
        {
            data[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
        else
        {
            ESP_LOGI(RX_TASK_TAG, "Nothing to read");
        }
    }
    free(data);
}

void app_main(void)
{
    init();
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES,
                NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL,
                configMAX_PRIORITIES - 1, NULL);
}
