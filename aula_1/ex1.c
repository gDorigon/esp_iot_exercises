#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#define PIN_SWITCH 15
#define PIN_LED 23
const char *TAG = "input.c";

void app_main(void)
{
    bool isOn = 0;
    printf("Switch and LED\n");
    ESP_LOGI(TAG, "Exercicio 1 rodando");
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_SWITCH);
    gpio_set_direction(PIN_SWITCH, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pulldown_en(PIN_SWITCH);
    gpio_pullup_dis(PIN_SWITCH);
    int level = 0;

    while (1)
    {
        level = gpio_get_level(PIN_SWITCH);
        if (gpio_get_level(PIN_SWITCH) == 1)
        {
            isOn = !isOn;
            gpio_set_level(PIN_LED, isOn);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            isOn = !isOn;
            gpio_set_level(PIN_LED, isOn);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
