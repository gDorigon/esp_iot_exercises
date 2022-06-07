#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#define PIN_SWITCH 15
#define PIN_LED 25
const char *TAG = "input.c";

bool isOn = 0;

void task1(void *param)
{
    while (true)
    {
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
    vTaskDelete(NULL);

}

void app_main(void)
{

    printf("Switch and LED\n");
    ESP_LOGI(TAG, "Exercicio 2 rodando");
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_SWITCH);
    gpio_set_direction(PIN_SWITCH, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pulldown_en(PIN_SWITCH);
    gpio_pullup_dis(PIN_SWITCH);

    xTaskCreate(&task1, "pin state", 2048, NULL, 1, NULL);
}

//by Carlos
