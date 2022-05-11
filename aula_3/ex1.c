#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define PIN_SWITCH 15
#define PIN_LED 23

xQueueHandle queue;

int level = 0, isOn = 0;

void fastf(void *params)
{
    int f = 0;
    xQueueReceive(queue, &f, 0);
    if (f == 1)
    {
        printf("FAST MODE");
        while (true)
        {
            isOn = !isOn;
            gpio_set_level(PIN_LED, isOn);
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

void slowf(void *params)
{
    int s = 0;
    xQueueReceive(queue, &s, 0);
    if (s == 0)
    {
        printf("SLOW MODE");
        while (true)
        {
            isOn = !isOn;
            gpio_set_level(PIN_LED, isOn);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

void buttonlvlsend(void *params)
{
    while (true)
    {
        level = gpio_get_level(PIN_SWITCH);
        xQueueSend(queue, &level, 10);
        printf("BOTAO PRESSIONADO");
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void app_main(void)
{
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(PIN_SWITCH);
    gpio_set_direction(PIN_SWITCH, GPIO_MODE_INPUT);
    gpio_pad_select_gpio(PIN_LED);
    gpio_set_direction(PIN_LED, GPIO_MODE_OUTPUT);
    gpio_pulldown_en(PIN_SWITCH);
    gpio_pullup_dis(PIN_SWITCH);
    queue = xQueueCreate(3, sizeof(int));
    xTaskCreate(&fastf, "fast frequency", 2048, NULL, 2, NULL);
    xTaskCreate(&slowf, "slow frequency", 2048, NULL, 1, NULL);
    xTaskCreate(&buttonlvlsend, "pin state", 2048, NULL, 1, NULL);
}
