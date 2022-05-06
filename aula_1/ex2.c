#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#define PIN_SWITCH 15
#define PIN_LED 23
const char *TAG = "input.c";

#define DELAYZIN 10

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
    bool cnt = 0;
    while (1)
    {
        if (gpio_get_level(PIN_SWITCH) == 1)
        {
            // vTaskDelay(pdMS_TO_TICKS(10));
            if (gpio_get_level(PIN_SWITCH) == 0)
            {
                cnt = !cnt;
            }
        }
        gpio_set_level(PIN_LED, 1);
        if (cnt == 1)
        {
            for (int i = 0; i < 200000; i++)
            {
                // vTaskDelay(pdMS_TO_TICKS(DELAYZIN));
                if (gpio_get_level(PIN_SWITCH) == 1)
                {
                    // vTaskDelay(pdMS_TO_TICKS(DELAYZIN));

                    if (gpio_get_level(PIN_SWITCH) == 0)
                    {
                        cnt = !cnt;
                    }
                }
            }

            gpio_set_level(PIN_LED, 0);
            for (int i = 0; i < 200000; i++)
            {
                // vTaskDelay(pdMS_TO_TICKS(DELAYZIN));
                if (gpio_get_level(PIN_SWITCH) == 1)
                {
                    // vTaskDelay(pdMS_TO_TICKS(DELAYZIN));

                    if (gpio_get_level(PIN_SWITCH) == 0)
                    {
                        cnt = !cnt;
                    }
                }
            }
        }
    }
}
